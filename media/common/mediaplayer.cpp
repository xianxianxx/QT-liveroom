#include "mediaplayer.h"
#include "mediacodec.h"
#include "mediatimer.h"
#include "mediautil.h"

#include <QDebug>
#include <QAudioDeviceInfo>

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent), widget(nullptr), audioOutput(nullptr), audioDevice(nullptr)
{
    processor = new MediaProcessor(this);

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    audioFormat = deviceInfo.preferredFormat();
    audioFormat.setSampleRate(44100);
    audioFormat.setSampleSize(16);
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    audioFormat.setChannelCount(2);

    if(deviceInfo.isFormatSupported(audioFormat) == false) {
        qInfo() << "[MediaPlayer::MediaPlayer] 音频格式调整";
        audioFormat = deviceInfo.nearestFormat(audioFormat);
    }

    audioOutput = new QAudioOutput(deviceInfo, audioFormat);

    vUtil.init(1280, 768, AV_PIX_FMT_RGB32,
              1280, 768, AV_PIX_FMT_RGB32);

    AVSampleFormat format = AV_SAMPLE_FMT_NONE;
    switch(audioFormat.sampleType()) {
    case QAudioFormat::Unknown:
        break;
    case QAudioFormat::SignedInt:
        format = (audioFormat.sampleSize() == 16 ? AV_SAMPLE_FMT_S16 : AV_SAMPLE_FMT_S32);
        break;
    case QAudioFormat::UnSignedInt:
        format = AV_SAMPLE_FMT_U8;
        break;
    case QAudioFormat::Float:
        format = AV_SAMPLE_FMT_FLT;
        break;
    }

    aUtil.init(av_get_default_channel_layout(audioFormat.channelCount()),
               audioFormat.sampleRate(), 1024, format,
               av_get_default_channel_layout(audioFormat.channelCount()),
               audioFormat.sampleRate(), format);

    QObject::connect(this, &MediaPlayer::transmitVideoFrame, this, &MediaPlayer::onVideoFramePrivate, Qt::QueuedConnection);
    QObject::connect(this, &MediaPlayer::transmitAudioFrame, this, &MediaPlayer::onAudioFramePrivate, Qt::QueuedConnection);
    QObject::connect(processor, &MediaProcessor::finished, this, &MediaPlayer::stop);
}

void MediaPlayer::setVideoOutput(MediaVideoWidget* widget)
{
    this->widget = widget;

    QObject::disconnect(processor, QOverload<AVFrame*>::of(&MediaProcessor::sendVideoFrame),
                     this, &MediaPlayer::onVideoFrame);
    QObject::connect(processor, QOverload<AVFrame*>::of(&MediaProcessor::sendVideoFrame),
                     this, &MediaPlayer::onVideoFrame);
}

void MediaPlayer::play()
{
    if(content.isValid() == false){
        qWarning() << "[MediaPlayer::play] 无效的媒体文件";
        return;
    }

    if(processor->load(content) == false) {
        qWarning() << "[MediaPlayer::play] 加载媒体文件失败";
        return;
    }

    if(audioDevice) {
        qWarning() << "[MediaPlayer::play] 正在播放";
        return;
    }

    switch(content.getType()) {
    case MediaContent::media:
    case MediaContent::camera:
    case MediaContent::microphone:
    case MediaContent::device:
        break;
    case MediaContent::desktop:
        qWarning() << "[MediaPlayer::play] 格式暂不支持 device desktop";
        return;
    }

    audioDevice = audioOutput->start();
    QObject::connect(processor, QOverload<AVFrame*>::of(&MediaProcessor::sendAudioFrame),
                     this, &MediaPlayer::onAudioFrame);

    processor->start();
}

void MediaPlayer::stop()
{
    processor->stop();
    QObject::disconnect(processor, QOverload<AVFrame*>::of(&MediaProcessor::sendAudioFrame),
                        this, &MediaPlayer::onAudioFrame);

    audioOutput->stop();
    audioDevice = nullptr;

    if(widget){
        QImage image(widget->width(), widget->height(), QImage::Format_RGB32);
        image.fill(Qt::black);
        widget->setImage(image);
    }
}

void MediaPlayer::pause()
{

}

void MediaPlayer::onVideoFrame(AVFrame* frame)
{
    if(widget == nullptr) return;
    emit transmitVideoFrame(av_frame_clone(frame));
}

void MediaPlayer::onAudioFrame(AVFrame* frame)
{
    if(audioDevice == nullptr) return;
    emit transmitAudioFrame(av_frame_clone(frame));
}

void MediaPlayer::onVideoFramePrivate(AVFrame* frame)
{
    widget->setImage(vUtil.transform(frame));
    av_frame_unref(frame);
    av_frame_free(&frame);
}

void MediaPlayer::onAudioFramePrivate(AVFrame* frame)
{
    auto array = aUtil.transform(frame);
    qDebug() << array.size();
    audioDevice->write(array);
    av_frame_unref(frame);
    av_frame_free(&frame);
}
