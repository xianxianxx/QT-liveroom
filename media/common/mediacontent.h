#ifndef MEDIACONTENT_H
#define MEDIACONTENT_H

#include <QUrl>
#include <QCameraInfo>
#include <QAudioDeviceInfo>
#include <QIODevice>

extern "C"
{
#include "libavformat/avformat.h"
}

class MediaIODevice;

class MediaContent
{
friend class MediaProcessor;

public:
    enum MediaType{
        media,      // 普通文件
        camera,     // 摄像头
        microphone, // 麦克风
        desktop,    // 桌面捕捉
        device      // io流
    };

    MediaContent(const QUrl& url = QUrl());
    MediaContent(const QCameraInfo& info);
    MediaContent(const QAudioDeviceInfo& info);
    MediaContent(MediaIODevice* device);

    inline MediaType getType() const{ return this->type; }
    inline bool isValid() const{ return !(url.isEmpty()); }

private:
    QUrl url;           // 媒体流路径
    MediaType type;     // 类型

    MediaIODevice* d;
};

#endif // MEDIACONTENT_H
