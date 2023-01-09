#ifndef MEDIARECORDER_H
#define MEDIARECORDER_H

#include <QObject>
#include <QFile>
#include <QThread>

#include "mediacontent.h"
#include "mediautil.h"

#include "mediaprocessor.h"
#include "mediamuxer.h"

class MediaTimer;
class MediaVideoWidget;
class MediaIODevice;

class MediaRecorder : public QObject
{
    Q_OBJECT
public:
    explicit MediaRecorder(QObject *parent = nullptr);
    virtual ~MediaRecorder();

    void setMedia(const MediaContent& video, const MediaContent& audio);
    void setVideoOutput(MediaVideoWidget* widget);
    inline bool isPreviewing() const{ return videoProcessor->isStart() && audioProcessor->isStart(); }
    inline bool isRecording() const{ return muxer->isRunning(); }

public slots:
    void preview();
    void stopPreview();
    void record(MediaIODevice* device = nullptr);

    void stopRecord();

private slots:
    void onVideoFrame(AVFrame* frame);
    void onAudioFrame(AVFrame* frame);

    void onVideoFrameToMuxer(int64_t timestamp, AVFrame* frame);
    void onAudioFrameToMuxer(int64_t timestamp, AVFrame* frame);

signals:
    void dbPresent(int db);
    void sendFrame(int idx, int64_t timestamp, AVFrame* frame);
    void sendPacket(AVPacket* packet);

private:
    MediaContent videoContent;
    MediaContent audioContent;

    MediaProcessor* videoProcessor;
    MediaProcessor* audioProcessor;

    MediaVideoWidget* widget;

    MediaMuxer* muxer;

    MediaVideoUtil vUtil;

    MediaIODevice* device;
};

#endif // MEDIARECODER_H
