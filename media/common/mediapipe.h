#ifndef MEDIAPIPE_H
#define MEDIAPIPE_H

extern "C"
{
#include "libavformat/avformat.h"
}

#include <QThread>
#include <QVector>
#include <QSemaphore>
#include <QWaitCondition>
#include <QMutex>
#include <QEventLoop>

#include "mediacontent.h"
#include "mediastream.h"

class MediaCodecDecoder;
class MediaIODevice;
class MediaTimer;

class MediaPipe : public QThread
{
    Q_OBJECT
public:
    MediaPipe(QObject* parent = nullptr);
    ~MediaPipe();

    bool load(const QVector<std::pair<AVCodecParameters*, AVRational>>& pars, MediaIODevice* device);
    inline bool isLoaded() const{ return fmtCtx != nullptr; }

    bool isStart(){ return isRunning(); }
    bool start();
    void stop();

    bool tryDecode();

    qint64 write(const QByteArray& array);

private slots:
    void threadWait();
    void onDeviceReadyRead();

signals:
    void sendFrame(int64_t timestamp, AVFrame* frame);
    void sendVideoFrame(AVFrame* frame);
    void sendAudioFrame(AVFrame* frame);
    void sendVideoFrame(int64_t timestamp, AVFrame* frame);
    void sendAudioFrame(int64_t timestamp, AVFrame* frame);

protected:
    virtual void run() override;

private:
    int decode(int count = 1);

private:
    AVFormatContext* fmtCtx;
    AVIOContext* avio;

    AVPacket* packet;

    MediaIODevice* device;

    QVector<QPair<MediaStream, MediaCodecDecoder*>> streams;
    QVector<MediaTimer*> timers;

    bool threadExitFlag;

    QSemaphore sem;

    QEventLoop* loop;
};

#endif // MEDIAPROCESSOR_H
