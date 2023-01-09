#include "mediapipe.h"
#include "mediacodec.h"
#include "mediaiodevice.h"
#include "mediaiodevice.h"

extern "C"
{
#include "libavformat/avio.h"
}

#include <QDebug>
#include <QCoreApplication>

MediaPipe::MediaPipe(QObject* parent)
    : QThread(parent), fmtCtx(nullptr), avio(nullptr), packet(nullptr), device(nullptr),
      loop(new QEventLoop(this))
{
    packet = av_packet_alloc();

    QObject::connect(this, &MediaPipe::finished,
                     this, &MediaPipe::threadWait);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [=](){
        if(isRunning()) {
            // 等待线程结束
            stop();

            QObject::connect(this, &MediaPipe::finished, [=](){
                loop->exit(0);
            });

            loop->exec();
        }
    });
}

MediaPipe::~MediaPipe()
{
    stop();

    if(packet) {
        av_packet_unref(packet);
        av_packet_free(&packet);
    }
}

bool MediaPipe::load(const QVector<std::pair<AVCodecParameters*, AVRational>>& pars, MediaIODevice* device)
{
    if(fmtCtx != nullptr) {
        qWarning() << "[MediaProcessor::load] 重复加载";
        return false;
    }

    bool ret = false;

    // 打开媒体
    const AVInputFormat* fmt = nullptr;
    AVDictionary* dict = nullptr;

    do{
        if(isLoaded()) {
            qWarning() << "[MediaPipe::load] 加载媒体内容失败，已经加载";
            break;
        }

        fmt = av_find_input_format("mpeg");
        if(fmt == nullptr) {
            qWarning() << "[MediaPipe::load] mpeg格式无法加载";
            break;
        }

        if(avio) {
            av_freep(&(avio->buffer));
            avio_context_free(&avio);
        }

        auto readFunc = [](void *opaque, uint8_t *buf, int buf_size)->int{
            MediaIODevice* device = (MediaIODevice*)opaque;
            if(device == nullptr || device->isOpen() == false) return -1;

//            if(device->waitForReadyRead(300) == false) return 0;
            int ret = device->read((char*)buf, buf_size);
//            if(ret > 0) qDebug() << "pipe : bytesRead " << ret;
            return ret;
        };

        avio = avio_alloc_context((uint8_t*)av_malloc(2048), 2048, 0, device, readFunc, nullptr, nullptr);
        if(avio == nullptr) {
            qWarning() << "[MediaPipe::load] 申请avio失败";
            break;
        }

        fmtCtx = avformat_alloc_context();
        fmtCtx->iformat = fmt;

        fmtCtx->pb = avio;
//        fmtCtx->probesize = 20480;
        fmtCtx->flags = (AVFMT_FLAG_FLUSH_PACKETS | AVFMT_FLAG_NOBUFFER);
        if(avformat_open_input(&fmtCtx, nullptr, fmt, &dict) < 0) {
            qWarning() << "[MediaPipe::load()] 打开formatContext失败";
            break;
        }

        av_dict_free(&dict);
    }while(0);

    this->device = device;
    this->device->open();

    QObject::connect(device, &MediaIODevice::readyRead, this, &MediaPipe::onDeviceReadyRead);

    ret = true;
    return ret;
}

bool MediaPipe::start()
{
    if(fmtCtx == nullptr || isRunning()) return false;

    QThread::start();
    return true;
}

void MediaPipe::stop()
{
    if(!isRunning()) return;

    QObject::disconnect(device, &MediaIODevice::readyRead, this, &MediaPipe::onDeviceReadyRead);

    threadExitFlag = false;

    device->close();
    device = nullptr;

    sem.release();
}

bool MediaPipe::tryDecode()
{
    if(isRunning() == false) return false;

    if(sem.available() == 0) sem.release();      // 释放processor线程

    return true;
}

qint64 MediaPipe::write(const QByteArray& array)
{
    if(isStart() == false) return -1;
    if(device->isOpen() == false) return -1;

    qint64 ret = device->write(array);
    tryDecode();

    return ret;
}

void MediaPipe::threadWait()
{
    wait();
    qDebug() << "[MediaPipe::threadWait] 管道回收";
}

void MediaPipe::onDeviceReadyRead()
{
    tryDecode();
}

void MediaPipe::run()
{
    QThread::sleep(2);

    qDebug() << avformat_find_stream_info(fmtCtx, nullptr);
    qDebug() << fmtCtx->nb_streams;

    // 获取媒体信息
    qDebug() << "[MediaPipe::load] 媒体流: " << fmtCtx->nb_streams;

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

            QObject::connect(stream.second, &MediaCodecDecoder::sendFrame, this, &MediaPipe::sendFrame, Qt::BlockingQueuedConnection);
        }
    }

    qDebug() << "[MediaPipe::run()] 管道开启";
    threadExitFlag = true;
    while(threadExitFlag) {
//        sem.acquire();

        if(threadExitFlag == false) break;
        if(decode() < 0){
            break;
        }
    }

    for(auto& p : streams) if(p.second) p.second->deleteLater();
    streams.clear();


    if(fmtCtx) {
        avformat_close_input(&fmtCtx);
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
    }

    if(avio) {
        av_freep(&(avio->buffer));
        avio_context_free(&avio);
    }

    for(auto& p : streams) if(p.second) p.second->deleteLater();
    streams.clear();

    qInfo() << "[MediaPipe::run()] 管道结束";
}

int MediaPipe::decode(int count)
{
    int ret = 0;

    for(int i = 0; i < count; ++i) {
        av_packet_unref(packet);
        int r = 0;
        if((r = av_read_frame(fmtCtx, packet)) < 0) {
//            qDebug() << ret;
            ret = -1;
            break;
        }else {
//            qDebug() << packet->pts << ' ' << packet->dts << ' '
//                     << packet->stream_index << ' ' << packet->size;
        }

        int idx = packet->stream_index;
        if(streams[idx].second) {
            ret += streams[idx].second->decode(av_packet_clone(packet));
        }
    }

    return ret;
}
