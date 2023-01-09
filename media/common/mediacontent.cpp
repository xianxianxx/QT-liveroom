#include "mediacontent.h"

#include <QDebug>

MediaContent::MediaContent(const QUrl& url)
    : url(url), type(media), d(nullptr)
{

}

MediaContent::MediaContent(const QCameraInfo& info)
    : url(QUrl::fromLocalFile("video=" + info.description())), type(camera), d(nullptr)
{

}

MediaContent::MediaContent(const QAudioDeviceInfo& info)
    : url(QUrl::fromLocalFile("audio=" + info.deviceName())), type(microphone), d(nullptr)
{

}

MediaContent::MediaContent(MediaIODevice* device)
    : url(QUrl::fromLocalFile("device=")), type(MediaContent::device), d(device)
{

}
