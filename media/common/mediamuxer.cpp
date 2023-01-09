#include "mediamuxer.h"

#include "mediacodec.h"
#include "mediautil.h"
#include "mediaqueue.h"
#include "mediaiodevice.h"

#include <QCoreApplication>

MediaMuxer::MediaMuxer(QObject *parent)
    : QThread(parent), fmtCtx(nullptr), avio(nullptr), exitFlag(false), loop(new QEventLoop(this)), device(nullptr)
{
    QObject::connect(this, &MediaMuxer::finished, this, &MediaMuxer::threadWait);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [=](){
        if(isRunning()) {
            // 等待线程结束
            close();

            QObject::connect(this, &MediaMuxer::finished, [=](){
                loop->exit(0);
            });

            loop->exec();
        }
    });
}

bool MediaMuxer::open(const QVector<std::pair<std::pair<AVCodecParameters*, AVCodecParameters*>, AVRational>>& pars, MediaIODevice* device)
{
    if(fmtCtx != nullptr) {
        qWarning() << "[MediaMuxer::open] 重复打开";
        return false;
    }

    if(device) device->open();

    avformat_alloc_output_context2(&fmtCtx, nullptr, "mpeg", nullptr);
    if(fmtCtx == nullptr) {
        qWarning() << "[MediaMuxer::open] 申请输出解码上下文失败";
        return false;
    }

    fmtCtx->flags = (AVFMT_FLAG_FLUSH_PACKETS | AVFMT_FLAG_NOBUFFER);

    encoders.resize(pars.size());

    for(int i = 0; i < pars.size(); ++i) {
        if(pars[i].first.first->codec_type != AVMEDIA_TYPE_VIDEO && pars[i].first.first->codec_type != AVMEDIA_TYPE_AUDIO) {
            qWarning() << "[MediaMuxer::open] 解码器类型错误";
            continue;
        }

        encoders[i].encoder = new MediaCodecEncoder(*(pars[i].first.first), pars[i].second, {1, 90000}, this);

        if(encoders[i].encoder->ctx == nullptr) {
            qWarning() << "[MediaMuxer::open] 申请解码器失败";
            continue;
        }

        AVStream* stream = avformat_new_stream(fmtCtx, nullptr);
        if(stream) {
            if(avcodec_parameters_from_context(stream->codecpar, encoders[i].encoder->ctx) < 0) {
                qWarning() << "[MediaMuxer::open] 拷贝stream参数失败";
                continue;
            }
        }else {
            qWarning() << "[MediaMuxer::open] 创建stream失败";
            continue;
        }

        stream->time_base = {1, 90000};
        encoders[i].queue = new MediaQueue;

        if(pars[i].first.first->codec_type == AVMEDIA_TYPE_VIDEO) {
            MediaVideoUtil* util = new MediaVideoUtil;
            util->init(pars[i].first.second->width, pars[i].first.second->height, (AVPixelFormat)(pars[i].first.second->format),
                       encoders[i].encoder->ctx->width, encoders[i].encoder->ctx->height, encoders[i].encoder->ctx->pix_fmt);
            encoders[i].util = util;
        }else if(pars[i].first.first->codec_type == AVMEDIA_TYPE_AUDIO) {
            MediaAudioUtil* util = new MediaAudioUtil;
            util->init(pars[i].first.second->channel_layout, pars[i].first.second->sample_rate, pars[i].first.second->sample_rate, (AVSampleFormat)(pars[i].first.second->format),
                       encoders[i].encoder->ctx->channel_layout, encoders[i].encoder->ctx->sample_rate, encoders[i].encoder->ctx->sample_fmt);
            encoders[i].util = util;
        }
    }

    auto writeFunc = [](void *opaque, uint8_t *buf, int size)->int{
        MediaMuxer* muxer = (MediaMuxer*)opaque;
        if(muxer == nullptr) return -1;

        MediaIODevice* device = muxer->device;
        QBuffer* buffer = &(muxer->buffer);

        if(device == nullptr || device->isOpen() == false) return -1;

        int ret = buffer->write((const char*)buf, size);
        if(buffer->pos() >= 32768) {
            device->write(buffer->data().left(buffer->pos()));
            buffer->seek(0);
        }

        if(ret > 0) qDebug() << "muxer written : " << ret;
        return ret;
    };

    this->device = device;
    avio = avio_alloc_context((uint8_t*)av_malloc(20480), 20480, 1, this, nullptr, writeFunc, nullptr);
    if(avio == nullptr) {
        qWarning() << "[MediaMuxer::open] 申请avio失败";
    }else {
//        avio->direct = 1;
//        fmtCtx->flush_packets = 1;
//        fmtCtx->packet_size = 20480;
        fmtCtx->pb = avio;
    }

    if(avformat_write_header(fmtCtx, nullptr) < 0) {
        qWarning() << "[MediaMuxer::open] 无法写入format header";
        close();
        return false;
    }

    exitFlag = true;
    buffer.open(QBuffer::WriteOnly);

    QThread::start();
    return true;
}

void MediaMuxer::close()
{
    if(isRunning() == false) return;

    buffer.close();
    exitFlag = false;
    sem.release();
}

int MediaMuxer::encode()
{
    int idx = -1;

    for(int i = 0; i < encoders.size(); ++i) {
        if(encoders[i].queue->empty()) continue;

        idx = (idx == -1 || encoders[i].queue->nextFrameTimestamp() < encoders[idx].queue->nextFrameTimestamp() ? i : idx);
    }

    if(idx == -1) return 0;
    if(encoders[idx].util == nullptr || encoders[idx].encoder == nullptr) {
        qWarning() << "[MediaMuxer::encode] 解码器不全";
        return -1;
    }

    auto p = encoders[idx].queue->dequeue();
    AVFrame* frame = p.second;
    AVFrame* newFrame = nullptr;

    int ret = 0;
    do{
        newFrame = encoders[idx].util->convert(frame);
        if(newFrame == nullptr) {
            qWarning() << "[MediaMuxer::encode] 转换格式失败";
            break;
        }

        QList<AVPacket*> packets;
        if(encoders[idx].encoder->encode(av_frame_clone(newFrame), p.first, packets) < 0){
            qWarning() << "[MediaMuxer::encode] 转码失败";
            break;
        }

        for(auto& packet : packets){
            packet->stream_index = idx;

            if(packet->pts < 0) continue;

            emit sendPacket(packet);
            if(av_interleaved_write_frame(fmtCtx, packet) < 0) {
                qWarning() << "[MediaMuxer::encode] 无法写入输出设备";
            }

            avio_flush(avio);
            if(buffer.pos() > 0){
                device->write(buffer.data().left(buffer.pos()));
                buffer.seek(0);
            }

            av_packet_unref(packet);
            av_packet_free(&packet);
        }

        ret = packets.size();
    }while(0);

    if(frame){
        av_frame_unref(frame);
        av_frame_free(&frame);
    }

    if(newFrame){
        if(newFrame->data[0]) av_free(newFrame->data[0]);
        av_frame_unref(newFrame);
        av_frame_free(&newFrame);
    }

    return ret;
}

void MediaMuxer::onFrameReceived(int idx, int64_t timestamp, AVFrame* frame)
{
    if(idx < 0 || idx >= encoders.size()){
        qWarning() << "[MediaMuxer::onFrameReceived] 媒体流编号超出范围" << idx;
        return;
    }

    if(encoders[idx].queue == nullptr) {
        qWarning() << "[MediaMuxer::onFrameReceivedPrivate] 无队列";
        return;
    }

    encoders[idx].queue->enqueue(timestamp, av_frame_clone(frame));
    sem.release();
}

void MediaMuxer::threadWait()
{
    QThread::wait();
    qDebug() << "[MediaMuxer::threadWait] 合成器回收";
}

void MediaMuxer::run()
{
    qDebug() << "[MediaMuxer::run] 合成器开始";
    while(exitFlag) {
        sem.acquire();
        if(exitFlag == false) break;
        int ret = 0;

        while((ret = encode()) > 0);
        if(exitFlag == false) break;
        if(ret < 0) break;
    }

    if(fmtCtx) {
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
    }

    if(avio) {
        av_freep(&(avio->buffer));
        avio_context_free(&avio);
    }

    for(auto encoder : encoders) {
        if(encoder.encoder) encoder.encoder->deleteLater();
        delete encoder.util;
        delete encoder.queue;
    }
    encoders.clear();

    qDebug() << "[MediaMuxer::run] 合成器结束";
}
