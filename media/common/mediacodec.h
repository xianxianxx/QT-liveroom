#ifndef MEDIACODEC_H
#define MEDIACODEC_H

extern "C"
{
#include "libavcodec/avcodec.h"
}

#include <QObject>

#include "mediastream.h"

class MediaCodecDecoder : public QObject
{
    friend class MediaPipe;

    Q_OBJECT
public:
    explicit MediaCodecDecoder(const MediaStream& stream, QObject *parent = nullptr);
    virtual ~MediaCodecDecoder();

    int decode(AVPacket* packet);
    int decode(AVPacket* packet, QList<AVFrame*>& frames);

private:
    void initVideoDecoder(const MediaStream& stream);
    void initAudioDecoder(const MediaStream& stream);
    int64_t guestPts();

signals:
    void sendFrame(int64_t, AVFrame* frame);

private:
    AVCodecContext* ctx;
    AVFrame* frame;
    AVFrame* hwFrame;
    AVBufferRef* buffer;

    AVPixelFormat pixFormat;
    AVRational timeBase;

    int64_t prevPts;
    int64_t avgFps;

    static QMap<AVCodecContext*, AVPixelFormat> pixFormats;
};

class MediaCodecEncoder : public QObject
{
friend class MediaMuxer;

    Q_OBJECT

public:
    MediaCodecEncoder(const AVCodecParameters& par, AVRational timeBase, AVRational streamTimebase, QObject *parent = nullptr);
    virtual ~MediaCodecEncoder();

    int encode(AVFrame* frame, int64_t timestamp);
    int encode(AVFrame* frame, int64_t timestamp, QList<AVPacket*>& packets);

private:
    // 为设置硬件加速
    void initVideoEncoder(const AVCodecParameters& par, AVRational timeBase);
    void initAudioEncoder(const AVCodecParameters& par, AVRational timeBase);

signals:
    void sendPacket(AVPacket* packet);

private:
    AVCodecContext* ctx;
    AVPacket* packet;

    int64_t startTime;
    AVRational streamTimebase;
};

#endif // MEDIACODEC_H
