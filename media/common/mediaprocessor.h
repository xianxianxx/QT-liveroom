#ifndef MEDIAPROCESSOR_H
#define MEDIAPROCESSOR_H

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
#include <QBuffer>

#include "mediacontent.h"
#include "mediastream.h"

class MediaCodecDecoder;
class MediaTimer;
class MediaIODevice;

class MediaProcessor : public QThread
{
friend class MediaPlayer;
friend class MediaRecorder;
friend class MediaTimer;

    Q_OBJECT
public:
    MediaProcessor(QObject* parent = nullptr);
    ~MediaProcessor();

    inline void setMedia(const MediaContent& content){ this->content = content; }
    inline bool load(const MediaContent& content) { setMedia(content); return load(); }
    bool load();
    bool isLoaded() const{ return fmtCtx != nullptr; }

    bool isStart(){ return isRunning(); }
    bool start(int64_t delay = 0);
    void stop();

public slots:
    bool tryDecode(bool wait);
    bool tryDecode();

private slots:
    void threadWait();
    void onIODeviceReadyRead();

signals:
    void sendFrame(int64_t timestamp, AVFrame* frame);
    void sendVideoFrame(AVFrame* frame);
    void sendAudioFrame(AVFrame* frame);
    void sendVideoFrame(int64_t timestamp, AVFrame* frame);
    void sendAudioFrame(int64_t timestamp, AVFrame* frame);
    void sendPacket(AVPacket* packet);

protected:
    virtual void run() override;

private:
    int decode(int count = 20);

    void initCamera(const char** filename, const AVInputFormat** fmt, AVDictionary** dict);
    void initMicrophone(const char** filename, const AVInputFormat** fmt, AVDictionary** dict);
    void initMedia(const char** filename, const AVInputFormat** fmt, AVDictionary** dict);
    void initDevice(const char** filename, const AVInputFormat** fmt, AVDictionary** dict);

private:
    MediaContent content;
    MediaIODevice* device;

    AVFormatContext* fmtCtx;
    AVPacket* packet;
    AVIOContext* avio;

    QVector<QPair<MediaStream, MediaCodecDecoder*>> streams;
    QVector<MediaTimer*> timers;

    bool threadExitFlag;

    QSemaphore sem;
    QWaitCondition cond;
    QMutex mutex;

    QEventLoop* loop;
    int64_t delay;

    QBuffer buffer;
};

#endif // MEDIAPROCESSOR_H
