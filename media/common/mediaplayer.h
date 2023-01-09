#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QObject>
#include <QVector>
#include <QAudioOutput>

#include "mediavideowidget.h"
#include "mediaprocessor.h"
#include "mediautil.h"

class MediaTimer;

class MediaPlayer : public QObject
{
    Q_OBJECT
public:
    explicit MediaPlayer(QObject *parent = nullptr);

    inline void setMedia(const MediaContent& content){ this->content = content; }
    void setVideoOutput(MediaVideoWidget* widget);

public slots:
    void play();
    void stop();
    void pause();

private slots:
    void onVideoFrame(AVFrame* frame);
    void onAudioFrame(AVFrame* frame);

    void onVideoFramePrivate(AVFrame* frame);
    void onAudioFramePrivate(AVFrame* frame);

signals:
    void transmitVideoFrame(AVFrame* frame);
    void transmitAudioFrame(AVFrame* frame);

private:
    MediaContent content;

    MediaProcessor* processor;
    MediaVideoWidget* widget;

    MediaVideoUtil vUtil;
    MediaAudioUtil aUtil;

    QAudioFormat audioFormat;
    QAudioOutput* audioOutput;
    QIODevice* audioDevice;
};

#endif // MEDIAPLAYER_H
