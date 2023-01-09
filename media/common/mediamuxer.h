#ifndef MEDIAMUXER_H
#define MEDIAMUXER_H

extern "C"
{
#include "libavformat/avformat.h"
}

#include <QIODevice>
#include <QThread>
#include <QLocalSocket>
#include <QLocalServer>
#include <QVector>
#include <QSemaphore>
#include <QEventLoop>
#include <QBuffer>

class MediaQueue;
class MediaUtil;
class MediaCodecEncoder;
class MediaIODevice;

class MediaMuxer : public QThread
{
    Q_OBJECT
private:
    struct Base{
        MediaCodecEncoder* encoder;
        MediaUtil* util;
        MediaQueue* queue;
        int64_t startTime;
        inline Base() : encoder(nullptr), util(nullptr), queue(nullptr), startTime(-1) {}
    };
public:
    explicit MediaMuxer(QObject *parent = nullptr);

    bool open(const QVector<std::pair<std::pair<AVCodecParameters*, AVCodecParameters*>, AVRational>>& pars, MediaIODevice* device);
    inline bool isOpen() const{ return fmtCtx != nullptr; }

    void close();

private:
    int encode();

public slots:
    void onFrameReceived(int idx, int64_t timestamp, AVFrame* frame);

private slots:
    void threadWait();

signals:
    void sendPacket(AVPacket* packet);

protected:
    virtual void run() override;

private:
    AVFormatContext* fmtCtx;
    AVIOContext* avio;

    QVector<Base> encoders;

    QSemaphore sem;
    bool exitFlag;

    QEventLoop* loop;

    MediaIODevice* device;
    QBuffer buffer;
};

#endif // MEDIAMUXER_H
