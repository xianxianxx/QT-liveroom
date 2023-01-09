#ifndef MEDIAUTIL_H
#define MEDIAUTIL_H

extern "C"{
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
};

#include <QImage>
#include <QByteArray>
#include <QMutex>

class MediaUtil
{
public:
    virtual ~MediaUtil() {}
    virtual AVFrame* convert(AVFrame* frame)=0;
};

class MediaVideoUtil : public MediaUtil
{
public:
    MediaVideoUtil();
    ~MediaVideoUtil();

    void init(int srcW, int srcH, AVPixelFormat srcFormat,
              int dstW, int dstH, AVPixelFormat dstFormat);

    QImage transform(AVFrame* frame);
    virtual AVFrame* convert(AVFrame* frame) override;

private:
    SwsContext* context; // 图片格式转换器

    int srcW;
    int srcH;
    AVPixelFormat srcFormat;
    int dstW;
    int dstH;
    AVPixelFormat dstFormat;

    QMutex mutex;

    uint8_t* dstData[4];
    int linesize[4];
};

class MediaAudioUtil : public MediaUtil
{
public:
    MediaAudioUtil();
    ~MediaAudioUtil();

    void init(int srcLayout, int srcRate, int srcSample, AVSampleFormat srcFormat,
              int dstLayout, int dstRate, AVSampleFormat dstFormat);

    QByteArray transform(AVFrame* frame);
    virtual AVFrame* convert(AVFrame* frame) override;

private:
    SwrContext* context; // 音频格式转换器

    int srcLayout;
    int srcRate;
    int srcSample;
    AVSampleFormat srcFormat;
    int dstLayout;
    int dstRate;
    int dstSample;
    AVSampleFormat dstFormat;

    QMutex mutex;

    uint8_t* dstData[4];
    int linesize[4];
};

#endif // MEDIAUTIL_H
