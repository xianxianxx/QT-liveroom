#include "mediarecorder.h"
#include "mediatimer.h"
#include "mediamuxer.h"
#include "mediavideowidget.h"
#include "mediaiodevice.h"

MediaRecorder::MediaRecorder(QObject *parent)
    : QObject(parent), widget(nullptr), device(nullptr)
{
    videoProcessor = new MediaProcessor(this);
    audioProcessor = new MediaProcessor(this);

    QObject::connect(audioProcessor, QOverload<AVFrame*>::of(&MediaProcessor::sendAudioFrame),
                     this, &MediaRecorder::onAudioFrame);

    muxer = new MediaMuxer;

    vUtil.init(1280, 768, AV_PIX_FMT_RGB32,
               1280, 768, AV_PIX_FMT_RGB32);

    QObject::connect(this, &MediaRecorder::sendFrame,
                     muxer, &MediaMuxer::onFrameReceived);
    QObject::connect(muxer, &MediaMuxer::sendPacket,
                     this, &MediaRecorder::sendPacket, Qt::BlockingQueuedConnection);
}

MediaRecorder::~MediaRecorder()
{
    muxer->deleteLater();
}

void MediaRecorder::setMedia(const MediaContent& video, const MediaContent& audio)
{
    if(video.getType() != MediaContent::camera) {
        qWarning() << "[MediaRecoder::setMedia] 视频媒体错误";
        return;
    }

    if(audio.getType() != MediaContent::microphone) {
        qWarning() << "[MediaRecoder::setMedia] 音频媒体错误";
        return;
    }

    videoContent = video;
    audioContent = audio;
}

void MediaRecorder::setVideoOutput(MediaVideoWidget* widget)
{
    this->widget = widget;

    QObject::disconnect(videoProcessor, QOverload<AVFrame*>::of(&MediaProcessor::sendVideoFrame),
                     this, &MediaRecorder::onVideoFrame);
    QObject::connect(videoProcessor, QOverload<AVFrame*>::of(&MediaProcessor::sendVideoFrame),
                     this, &MediaRecorder::onVideoFrame);
}

void MediaRecorder::preview()
{
    if(videoProcessor->isRunning() || audioProcessor->isRunning()) {
        return;
    }

    if(!videoContent.isValid()) {
        qWarning() << "[MediaRecoder::preview] 视频媒体文件无效，无法播放";
        return;
    }

    if(!videoContent.isValid()) {
        qWarning() << "[MediaRecoder::preview] 音频媒体文件无效，无法播放";
        return;
    }

    videoProcessor->setMedia(videoContent);
    audioProcessor->setMedia(audioContent);
    if(videoProcessor->load() == false){
        qWarning() << "[MediaRecoder::preview] 视频媒体文件无效，无法播放";
        return;
    }
    if(audioProcessor->load() == false){
        qWarning() << "[MediaRecoder::preview] 视频媒体文件无效，无法播放";
        return;
    }

    videoProcessor->start();
    audioProcessor->start();
}

void MediaRecorder::stopPreview()
{
    videoProcessor->stop();
    audioProcessor->stop();

    if(widget){
        QImage image(widget->width(), widget->height(), QImage::Format_RGB32);
        image.fill(Qt::black);
        widget->setImage(image);
    }
}

void MediaRecorder::record(MediaIODevice* device)
{
    if(isPreviewing() == false) {
        qWarning() << "[MediaRecorder::record] 必须先预览";
        return;
    }

    if(muxer->isRunning()) {
        qWarning() << "[MediaRecorder::record] 合成器已经运行";
        return;
    }

    if(device == nullptr) {
        qWarning() << "[MediaRecorder::record] 未设置设备";
    }

    this->device = device;

    QVector<std::pair<std::pair<AVCodecParameters*,AVCodecParameters*>, AVRational>> pars;
    if(videoProcessor->isStart()){
        AVRational timeBase = av_inv_q(videoProcessor->fmtCtx->streams[0]->avg_frame_rate);

        auto parDst = avcodec_parameters_alloc();
        parDst->codec_type = AVMEDIA_TYPE_VIDEO;
        parDst->codec_id = AV_CODEC_ID_H264;
        parDst->width = videoProcessor->fmtCtx->streams[0]->codecpar->width;
        parDst->height = videoProcessor->fmtCtx->streams[0]->codecpar->height;
        parDst->bit_rate = 400000;

        auto parSrc = avcodec_parameters_alloc();
        avcodec_parameters_copy(parSrc, videoProcessor->fmtCtx->streams[0]->codecpar);

        pars.append(std::make_pair(std::make_pair(parDst, parSrc), timeBase));
    }

    if(audioProcessor->isStart()){
        AVRational timeBase = av_inv_q(audioProcessor->fmtCtx->streams[0]->avg_frame_rate);

        auto parDst = avcodec_parameters_alloc();
        parDst->codec_type = AVMEDIA_TYPE_AUDIO;
        parDst->codec_id = AV_CODEC_ID_MP3;

        auto parSrc = avcodec_parameters_alloc();
        avcodec_parameters_copy(parSrc, audioProcessor->fmtCtx->streams[0]->codecpar);
        parSrc->channel_layout = av_get_default_channel_layout(parSrc->channels);

        pars.append(std::make_pair(std::make_pair(parDst, parSrc), timeBase));
    }

    if(muxer->open(pars, device) == false){
        qWarning() << "[MediaRecorder::record] 打开合成器失败";
        device->close();
        return;
    }

    for(auto& par : pars) {
        avcodec_parameters_free(&(par.first.first));
        avcodec_parameters_free(&(par.first.second));
    }

    QObject::connect(videoProcessor, QOverload<int64_t, AVFrame*>::of(&MediaProcessor::sendVideoFrame),
                     this, &MediaRecorder::onVideoFrameToMuxer);
    QObject::connect(audioProcessor, QOverload<int64_t, AVFrame*>::of(&MediaProcessor::sendAudioFrame),
                     this, &MediaRecorder::onAudioFrameToMuxer);
}

void MediaRecorder::stopRecord()
{
    QObject::disconnect(videoProcessor, QOverload<int64_t, AVFrame*>::of(&MediaProcessor::sendVideoFrame),
                     this, &MediaRecorder::onVideoFrameToMuxer);
    QObject::disconnect(audioProcessor, QOverload<int64_t, AVFrame*>::of(&MediaProcessor::sendAudioFrame),
                     this, &MediaRecorder::onAudioFrameToMuxer);

    muxer->close();
}

void MediaRecorder::onVideoFrame(AVFrame* frame)
{
    if(widget == nullptr) return;
    widget->setImage(vUtil.transform(frame));
}

void MediaRecorder::onAudioFrame(AVFrame* frame)
{
    int db = 0;
    short value = 0;
    double sum = 0;

    for(int i = 0; i < frame->linesize[0]; i += 2) {
        memcpy(&value, frame->data[0] + i, 2);
        sum += qAbs(value);
    }

    sum = sum / (frame->linesize[0] / 2);
    if(sum > 0) {
        db = (int)(20.0 * log10(sum));
    }

    emit dbPresent(db);
}

void MediaRecorder::onVideoFrameToMuxer(int64_t timestamp, AVFrame* frame)
{
    emit sendFrame(0, timestamp, frame);
}

void MediaRecorder::onAudioFrameToMuxer(int64_t timestamp, AVFrame* frame)
{
    emit sendFrame(1, timestamp, frame);
}
