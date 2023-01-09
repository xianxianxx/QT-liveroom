#include "mediautil.h"

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
}

#include <QDebug>
#include <QThread>
MediaVideoUtil::MediaVideoUtil()
    : context(nullptr), srcW(-1), srcH(-1), srcFormat(AV_PIX_FMT_NONE),
      dstW(-1), dstH(-1), dstFormat(AV_PIX_FMT_NONE),
      dstData{}, linesize{}
{

}

MediaVideoUtil::~MediaVideoUtil()
{
    if(context) sws_freeContext(context);
    if(dstData[0]) av_free(dstData[0]);
}

void MediaVideoUtil::init(int srcW, int srcH, AVPixelFormat srcFormat, int dstW, int dstH, AVPixelFormat dstFormat)
{
    if(srcW < 0 || srcH < 0 || srcFormat == AV_PIX_FMT_NONE
            || dstW < 0 || dstH < 0 || dstFormat == AV_PIX_FMT_NONE) return;

    mutex.lock();
    this->srcW = srcW;
    this->srcH = srcH;
    this->srcFormat = srcFormat;
    this->dstW = dstW;
    this->dstH = dstH;
    this->dstFormat = dstFormat;

    if(context) sws_freeContext(context);

    context = sws_getContext(srcW, srcH, srcFormat, dstW, dstH, dstFormat, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if(context == nullptr) {
        qWarning() << "[MediaVideoUtil::init] 获取sws失败";
        mutex.unlock();
        return;
    }

    if(dstData[0]) av_free(dstData[0]);

    // 申请目标图片
    if(av_image_alloc(dstData, linesize, dstW, dstH, dstFormat, 4) < 0) {
        qWarning() << "[MediaVideoUtil::init] 申请图片失败";
    }
    mutex.unlock();
}

QImage MediaVideoUtil::transform(AVFrame* frame)
{
    if(context == nullptr || srcW < 0 || srcH < 0 || srcFormat == AV_PIX_FMT_NONE
            || dstW < 0 || dstH < 0 || dstFormat == AV_PIX_FMT_NONE) return QImage();

    if(frame == nullptr) return {};

    if(srcW != frame->width || srcH != frame->height || srcFormat != frame->format)
        init(frame->width, frame->height, (AVPixelFormat)frame->format, dstW, dstH, dstFormat);

    mutex.lock();
    int ret = -1;
    if((ret = sws_scale(context, frame->data, frame->linesize, 0, srcH, dstData, linesize)) < 0) {
        qWarning() << "[MediaVideoUtil::transform] 图片转换失败 " << ret;
        mutex.unlock();
        return QImage();
    }

    QImage image(dstW, dstH, QImage::Format_RGB32);
    for(int y = 0; y < dstH; ++y) {
        memcpy(image.scanLine(y), dstData[0] + linesize[0] * y, linesize[0]);
    }

    mutex.unlock();
    return image;
}

AVFrame* MediaVideoUtil::convert(AVFrame* frame)
{
    if(context == nullptr || srcW < 0 || srcH < 0 || srcFormat == AV_PIX_FMT_NONE
            || dstW < 0 || dstH < 0 || dstFormat == AV_PIX_FMT_NONE) return nullptr;

    if(frame == nullptr) {
        return nullptr;
    }
    if(srcFormat != frame->format) init(srcW, srcH, (AVPixelFormat)frame->format, dstW, dstH, dstFormat);
//    qDebug() << srcW << ' ' << srcH << ' ' <<srcFormat;
//    qDebug() << dstW << ' ' << dstH << ' ' <<dstFormat;
    AVFrame* newFrame = nullptr;

    mutex.lock();

    newFrame = av_frame_alloc();
    av_image_alloc(newFrame->data, newFrame->linesize, dstW, dstH, dstFormat, 1);

    int ret = -1;
    if((ret = sws_scale(context, frame->data, frame->linesize, 0, srcH, newFrame->data, newFrame->linesize)) < 0) {
        qWarning() << "图片转换失败 " << ret;
        av_free(newFrame->data[0]);
        av_frame_unref(newFrame);
        av_frame_free(&newFrame);

        mutex.unlock();
        return nullptr;
    }

    newFrame->width = dstW;
    newFrame->height = dstH;
    newFrame->format = dstFormat;
    newFrame->pts = frame->pts;
    newFrame->pkt_dts = frame->pkt_dts;

    mutex.unlock();

    return newFrame;
}

MediaAudioUtil::MediaAudioUtil()
    : context(nullptr), srcLayout(-1), srcRate(-1), srcSample(-1),
      srcFormat(AV_SAMPLE_FMT_NONE), dstLayout(-1),
      dstRate(-1), dstFormat(AV_SAMPLE_FMT_NONE),
      dstData{}, linesize{}
{

}

MediaAudioUtil::~MediaAudioUtil()
{
    if(context) swr_free(&context);
    if(dstData[0]) av_free(dstData[0]);
}

void MediaAudioUtil::init(int srcLayout, int srcRate, int srcSample, AVSampleFormat srcFormat, int dstLayout, int dstRate, AVSampleFormat dstFormat)
{
    mutex.lock();
    this->srcLayout = srcLayout;
    this->srcRate = srcRate;
    this->srcSample = srcSample;
    this->srcFormat = srcFormat;
    this->dstLayout = dstLayout;
    this->dstRate = dstRate;
    this->dstFormat = dstFormat;
    this->dstSample = av_rescale_rnd(srcSample, dstRate, srcRate, AV_ROUND_UP);

//    qDebug() << srcLayout << ' ' << srcRate << ' ' << srcFormat;
//    qDebug() << dstLayout << ' ' << dstRate << ' ' << dstFormat;

    if(context) swr_free(&context);
    context = swr_alloc_set_opts(nullptr, dstLayout, dstFormat, dstRate,
                                 srcLayout, srcFormat, srcRate, 0, nullptr);
    if(context == nullptr) {
        qWarning() << "[MediaAudioUtil::init] 获取sws失败";
        mutex.unlock();
        return;
    }

    if(swr_init(context) < 0) {
        qWarning() << "[MediaAudioUtil::init] 初始化音频转换器失败";
        mutex.unlock();
        return;
    }

    if(dstData[0]) av_free(dstData[0]);

    // 申请目标音频帧
    if(av_samples_alloc(dstData, linesize, av_get_channel_layout_nb_channels(dstLayout),
                        dstSample, dstFormat, 1) < 0) {
        qWarning() << "[MediaAudioUtil::init] 申请音频段失败";
    }

    mutex.unlock();
}

QByteArray MediaAudioUtil::transform(AVFrame* frame)
{
    if(srcLayout < 0 || srcRate < 0 || srcSample < 0 || srcFormat == AV_SAMPLE_FMT_NONE
            || dstLayout < 0 || dstRate < 0 || dstFormat == AV_SAMPLE_FMT_NONE) return QByteArray();

    if(srcLayout != av_get_default_channel_layout(frame->channels) ||
            srcRate != frame->sample_rate || srcFormat != frame->format)
        init(av_get_default_channel_layout(frame->channels),
             frame->sample_rate, srcSample, (AVSampleFormat)frame->format,
             dstLayout, dstRate, dstFormat);

    if(srcSample != frame->nb_samples) srcSample = frame->nb_samples;

    // 计算sample size
    int ret = -1;
    int dstSample = av_rescale_rnd(swr_get_delay(context, srcRate) +
                                   srcSample, dstRate, srcRate, AV_ROUND_UP);

//    qDebug() << srcLayout << ' ' << srcRate << ' ' << srcFormat << ' ' << srcSample;
//    qDebug() << dstLayout << ' ' << dstRate << ' ' << dstFormat << ' ' << dstSample;

    if (dstSample > this->dstSample) {
        av_freep(&dstData[0]);
        ret = av_samples_alloc(dstData, linesize, av_get_channel_layout_nb_channels(dstLayout),
                               dstSample, dstFormat, 1);

        this->dstSample = dstSample;
    }

    if((ret = swr_convert(context, dstData, dstSample, (const uint8_t**)frame->data, srcSample)) < 0) {
        qWarning() << "[MediaAudioUtil::transform] 音频转换失败 " << ret;
        return QByteArray();
    }

    QByteArray array((char*)dstData[0], av_samples_get_buffer_size(nullptr, av_get_channel_layout_nb_channels(dstLayout),
                                                         ret, dstFormat, 1));
    return array;
}

AVFrame* MediaAudioUtil::convert(AVFrame* frame)
{
    if(srcLayout < 0 || srcRate < 0 || srcSample < 0 || srcFormat == AV_SAMPLE_FMT_NONE
            || dstLayout < 0 || dstRate < 0 || dstFormat == AV_SAMPLE_FMT_NONE) return nullptr;

    if(srcSample != frame->nb_samples) srcSample = frame->nb_samples;

    // 计算sample size
    int ret = -1;
    int dstSample = av_rescale_rnd(swr_get_delay(context, srcRate) +
                                   srcSample, dstRate, srcRate, AV_ROUND_UP);

//    qDebug() << srcLayout << ' ' << srcRate << ' ' << srcFormat << ' ' << srcSample;
//    qDebug() << dstLayout << ' ' << dstRate << ' ' << dstFormat << ' ' << dstSample;

    if (dstSample > this->dstSample) {
        this->dstSample = dstSample;
    }

    AVFrame* newFrame = av_frame_alloc();

    ret = av_samples_alloc(newFrame->data, newFrame->linesize, av_get_channel_layout_nb_channels(dstLayout),
                           dstSample, dstFormat, 1);

    if(ret < 0) {
        qWarning() << "[MediaAudioUtil::transform] 申请音频段失败 " << ret;
        av_freep(&(newFrame->data[0]));
        av_frame_free(&newFrame);
        return nullptr;
    }

    if((ret = swr_convert(context, newFrame->data, dstSample, (const uint8_t**)frame->data, srcSample)) < 0) {
        qWarning() << "[MediaAudioUtil::transform] 音频转换失败 " << ret;
        return nullptr;
    }

    newFrame->nb_samples = ret;
    newFrame->format = dstFormat;
    newFrame->channel_layout = dstLayout;
    newFrame->channels = av_get_channel_layout_nb_channels(dstLayout);
    newFrame->sample_rate = dstRate;
    newFrame->pts = frame->pts;
    newFrame->pkt_dts = frame->pkt_dts;

    return newFrame;
}
