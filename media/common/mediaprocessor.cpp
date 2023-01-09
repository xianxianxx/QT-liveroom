#include "mediaprocessor.h"
#include "mediacodec.h"
#include "mediatimer.h"
#include "mediaiodevice.h"

extern "C"
{
#include "libavformat/avio.h"
}

#include <QDebug>
#include <QCoreApplication>

MediaProcessor::MediaProcessor(QObject* parent)
    : QThread(parent), device(nullptr), fmtCtx(nullptr), packet(nullptr), avio(nullptr), loop(new QEventLoop(this))
{
    packet = av_packet_alloc();

    QObject::connect(this, &MediaProcessor::finished,
                     this, &MediaProcessor::threadWait);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [=](){
        if(isRunning()) {
            // 等待线程结束
            stop();

            QObject::connect(this, &MediaProcessor::finished, [=](){
                loop->exit(0);
            });

            loop->exec();
        }
    });
}

MediaProcessor::~MediaProcessor()
{
    stop();

    if(packet) {
        av_packet_unref(packet);
        av_packet_free(&packet);
    }
}

bool MediaProcessor::load()
{
    if(fmtCtx != nullptr) {
        qWarning() << "[MediaProcessor::load] 重复加载";
        return false;
    }

    bool ret = false;

    // 打开媒体
    const char* filename = content.url.toLocalFile().toStdString().data();
    const AVInputFormat* fmt = nullptr;
    AVDictionary* dict = nullptr;

    do{
        if(isLoaded()) {
            qWarning() << "[MediaProcessor::load()] 加载媒体内容失败，已经加载";
            break;
        }

        if(fmtCtx){
            avformat_free_context(fmtCtx);
            fmtCtx = nullptr;
        }

        fmtCtx = avformat_alloc_context();

        switch(content.type) {
        case MediaContent::camera:
            initCamera(&filename, &fmt, &dict);
            break;
        case MediaContent::microphone:
            initMicrophone(&filename, &fmt, &dict);
            break;
        case MediaContent::media:
            initMedia(&filename, &fmt, &dict);
            break;
        case MediaContent::device:
            initDevice(&filename, &fmt, &dict);
            break;
        case MediaContent::desktop:
            qWarning() << "[MediaProcessor::load] 格式暂不支持 desktop";
            return false;
        }

        if(avformat_open_input(&fmtCtx, filename, fmt, &dict) < 0) {
            qWarning() << "[MediaProcessor::load()] 打开formatContext失败";
            break;
        }

        av_dict_free(&dict);
    }while(0);

    ret = true;
    return ret;
}

bool MediaProcessor::start(int64_t delay)
{
    if(fmtCtx == nullptr || isRunning()) return false;

    this->delay = delay;
    QThread::start();

    return true;
}

void MediaProcessor::stop()
{
    if(!isRunning()) return;

    threadExitFlag = false;

    if(device) {
        device->close();
        QObject::disconnect(device, &MediaIODevice::readyRead, this, &MediaProcessor::onIODeviceReadyRead);
        device = nullptr;
    }
    sem.release();
}

bool MediaProcessor::tryDecode(bool wait)
{
    if(isRunning() == false) return false;

    if(sem.available() == 0) sem.release();      // 释放processor线程

    if(wait){
        mutex.lock();       // 等待processor线程完成工作
        cond.wait(&mutex);
        mutex.unlock();
    }

    return true;
}

bool MediaProcessor::tryDecode()
{
    return tryDecode(false);
}

void MediaProcessor::threadWait()
{
    wait();
    qDebug() << "[MediaProcessor::threadWait] 处理器回收";
}

void MediaProcessor::onIODeviceReadyRead()
{
    tryDecode();
}

void MediaProcessor::run()
{
    if(avio) QThread::sleep(1);

    if(avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        qWarning() << "[MediaProcessor::load()] 查询媒体流信息失败";
    }

    // 获取媒体信息
    qDebug() << "[MediaProcessor::load] 媒体流: " << fmtCtx->nb_streams;

    streams.resize(fmtCtx->nb_streams);
    for(unsigned int i = 0; i < fmtCtx->nb_streams; ++i) {
        auto& stream = streams[i];

        stream.first.idx = i;
        stream.first.stream = fmtCtx->streams[i];
        stream.second = nullptr;

        qDebug() << "[MediaProcessor::load] " << i << ':' << av_get_media_type_string(stream.first.getMediaType());
    }

    // 获取解码器
    for(unsigned int i = 0; i < fmtCtx->nb_streams; ++i) {
        auto& stream = streams[i];
        if(stream.first.getMediaType() == AVMEDIA_TYPE_VIDEO ||
           stream.first.getMediaType() == AVMEDIA_TYPE_AUDIO) {
            stream.second = new MediaCodecDecoder(stream.first);

            QObject::connect(stream.second, &MediaCodecDecoder::sendFrame,
                             this, &MediaProcessor::sendFrame, Qt::BlockingQueuedConnection);
        }
    }

    // 设置定时器

    timers.resize(streams.size());

    for(int i = 0; i < streams.size(); ++i) {
        AVStream* stream = streams[i].first.stream;
        int64_t startTime = av_rescale_q(stream->start_time, stream->time_base, AV_TIME_BASE_Q);

        timers[i] = new MediaTimer(startTime);
        QObject::connect(streams[i].second, &MediaCodecDecoder::sendFrame,
                         timers[i], &MediaTimer::enqueueFrame);
        QObject::connect(timers[i], &MediaTimer::needFrames,
                         this, QOverload<>::of(&MediaProcessor::tryDecode));

        switch(streams[i].first.stream->codecpar->codec_type){
        case AVMEDIA_TYPE_VIDEO:
            QObject::connect(timers[i], &MediaTimer::sendFrame, this,
                             QOverload<AVFrame*>::of(&MediaProcessor::sendVideoFrame), Qt::BlockingQueuedConnection);
            QObject::connect(streams[i].second, &MediaCodecDecoder::sendFrame, this,
                             QOverload<int64_t, AVFrame*>::of(&MediaProcessor::sendVideoFrame), Qt::BlockingQueuedConnection);
            break;
        case AVMEDIA_TYPE_AUDIO:
            QObject::connect(timers[i], &MediaTimer::sendFrame, this,
                             QOverload<AVFrame*>::of(&MediaProcessor::sendAudioFrame), Qt::BlockingQueuedConnection);
            QObject::connect(streams[i].second, &MediaCodecDecoder::sendFrame, this,
                             QOverload<int64_t, AVFrame*>::of(&MediaProcessor::sendAudioFrame), Qt::BlockingQueuedConnection);
            break;
        default:
            break;
        }
    }

    for(auto t : timers) t->start(delay);

    qDebug() << "[MediaProcessor::run()] 处理器开启";

    threadExitFlag = true;
    while(threadExitFlag) {
        sem.acquire();

        if(threadExitFlag == false) break;
        if(decode() < 0) break;
    }

    bool flag = true;
    while(flag) {
        flag = false;
        for(auto t : timers) flag = !(t->queue.empty());
        QThread::msleep(30);
    }

    // 结束定时器
    for(auto t : timers){
        cond.wakeAll();
        t->release();
        t->stop();
    }

    for(auto& p : streams) if(p.second) p.second->deleteLater();
    streams.clear();


    if(fmtCtx) {
        avformat_close_input(&fmtCtx);
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
    }

    for(auto& p : streams) if(p.second) p.second->deleteLater();
    streams.clear();

    for(auto& p : timers) if(p) p->deleteLater();
    timers.clear();

    qInfo() << "[MediaProcessor::stop] 处理器结束";
}

int MediaProcessor::decode(int count)
{
    int ret = 0;

    for(int i = 0; i < count; ++i) {
        av_packet_unref(packet);
        int r = 0;
        if((r = av_read_frame(fmtCtx, packet)) < 0) {
            if(r == AVERROR_EOF){
                qWarning() << "[MediaProcessor::decode] EOF";
                ret = -1;
            }else {
//                qDebug() << r;
            }
            break;
        }else emit sendPacket(packet);

        if(0 > packet->stream_index || streams.size() <= packet->stream_index) continue;

        if(streams[packet->stream_index].second) {
            ret += streams[packet->stream_index].second->decode(av_packet_clone(packet));
            if(0 <= packet->stream_index && packet->stream_index < timers.size()) timers[packet->stream_index]->release();
        }

        cond.wakeAll();
    }

    cond.wakeAll();

    return ret;
}

void MediaProcessor::initCamera(const char** filename, const AVInputFormat** fmt, AVDictionary** dict)
{
    Q_UNUSED(filename);
    *fmt = av_find_input_format("dshow");
    av_dict_set(dict, "video_size", "640x480", 0);
//    av_dict_set(dict, "video_size", "1280x720", 0);
}

void MediaProcessor::initMicrophone(const char** filename, const AVInputFormat** fmt, AVDictionary** dict)
{
    Q_UNUSED(filename);
    *fmt = av_find_input_format("dshow");
    av_dict_set(dict, "acodec", "aac", 0);
    av_dict_set(dict, "audio_buffer_size", "23.22", 0);
}

void MediaProcessor::initMedia(const char** filename, const AVInputFormat** fmt, AVDictionary** dict)
{
    Q_UNUSED(filename);
    Q_UNUSED(fmt);
    Q_UNUSED(dict);
}

void MediaProcessor::initDevice(const char** filename, const AVInputFormat** fmt, AVDictionary** dict)
{
    Q_UNUSED(dict);

    *filename = nullptr;
    *fmt = av_find_input_format("mpeg");
    if(fmt == nullptr) {
        qWarning() << "[MediaPipe::load] mpeg格式无法加载";
        return;
    }

    if(avio) {
        av_freep(&(avio->buffer));
        avio_context_free(&avio);
    }

    auto readFunc = [](void *opaque, uint8_t *buf, int buf_size)->int{
        MediaIODevice* device = (MediaIODevice*)opaque;
        if(device == nullptr || device->isOpen() == false) return -1;

//        if(device->waitForReadyRead(3000) == false) return 0;
        int ret = device->read((char*)buf, buf_size);
//        if(ret > 0) qDebug() << "pipe : bytesRead " << ret;
        return ret;
    };

    device = content.d;
    avio = avio_alloc_context((uint8_t*)av_malloc(20480), 20480, 0, device, readFunc, nullptr, nullptr);
    if(avio == nullptr) {
        qWarning() << "[MediaPipe::load] 申请avio失败";
        device = nullptr;
        return;
    }

    fmtCtx->probesize = 200000000;
//    fmtCtx->max_analyze_duration = 20000000;
    fmtCtx->iformat = *fmt;
    fmtCtx->pb = avio;
    fmtCtx->flags = (AVFMT_FLAG_FLUSH_PACKETS | AVFMT_FLAG_NOBUFFER);

    QObject::connect(device, &MediaIODevice::readyRead, this, &MediaProcessor::onIODeviceReadyRead);
    device->open();
}
