#include "mediacodec.h"

extern "C"
{
#include "libswresample/swresample.h"
}

#include <QDebug>

/** Media Decoder **/
QMap<AVCodecContext*, AVPixelFormat> MediaCodecDecoder::pixFormats;

MediaCodecDecoder::MediaCodecDecoder(const MediaStream& stream, QObject *parent)
    : QObject(parent), ctx(nullptr), frame(nullptr), hwFrame(nullptr),
      buffer(nullptr), pixFormat(AV_PIX_FMT_NONE), timeBase(stream.stream->time_base),
      prevPts(-1), avgFps(-1)
{
    if(stream.stream != nullptr){
        switch(stream.stream->codecpar->codec_type){
        case AVMEDIA_TYPE_VIDEO:
            initVideoDecoder(stream);
            break;
        case AVMEDIA_TYPE_AUDIO:
            initAudioDecoder(stream);
            break;
        default:
            break;
        }
    }

    frame = av_frame_alloc();
    hwFrame = av_frame_alloc();
}

MediaCodecDecoder::~MediaCodecDecoder()
{
    if(frame) {
        av_frame_unref(frame);
        av_frame_free(&frame);
    }

    if(hwFrame) {
        av_frame_unref(hwFrame);
        av_frame_free(&hwFrame);
    }

    if(ctx) avcodec_free_context(&ctx);
}

int MediaCodecDecoder::decode(AVPacket* packet)
{
    bool ret = false;
    do{
        if(ctx == nullptr) break;
        int ret = 0;
        if((ret = avcodec_send_packet(ctx, packet)) < 0){
//            qDebug() << ret;
            break;
        }

        while(1){
            av_frame_unref(frame);
            if(avcodec_receive_frame(ctx, frame) < 0){
                break;
            }

            if(frame->pts == AV_NOPTS_VALUE) {
                if(frame->pkt_dts != AV_NOPTS_VALUE) frame->pts = frame->pkt_dts;
                else if(prevPts != frame->pts) frame->pkt_dts = guestPts();
            }

            if(frame->pts != AV_NOPTS_VALUE) {
                if(prevPts >= 0){
                    int64_t f = frame->pts - prevPts;
                    if(f > 0 && avgFps < 0) avgFps = f;
                    else if(f > 0) avgFps = qMin(f, avgFps);
                }

                prevPts = frame->pts;
            }

            if(frame->pts == AV_NOPTS_VALUE) {
                if(frame->pkt_dts != AV_NOPTS_VALUE) frame->pts = frame->pkt_dts;
                else continue;
            }

            int64_t timestamp = (frame->pts == AV_NOPTS_VALUE ? AV_NOPTS_VALUE : av_rescale_q(frame->pts, timeBase, AV_TIME_BASE_Q));
            if(frame->format == pixFormat) {
                av_frame_unref(hwFrame);
                if(av_hwframe_transfer_data(hwFrame, frame, 0) < 0){
                    qWarning() << "[MediaCodecDecoder::decode] 硬件解码失败";
                    break;
                }

                av_frame_unref(frame);
                av_frame_ref(frame, hwFrame);
            }

            emit sendFrame(timestamp, frame);
            ret = true;
        }
    }while(0);

    av_packet_unref(packet);
    av_packet_free(&packet);

    return ret;
}

int MediaCodecDecoder::decode(AVPacket* packet, QList<AVFrame*>& frames)
{
    int ret = false;
    do{
        if(ctx == nullptr) break;
        if(avcodec_send_packet(ctx, packet) < 0) break;

        while(1){
            av_frame_unref(frame);
            if(avcodec_receive_frame(ctx, frame) < 0) break;

            if(frame->pts == AV_NOPTS_VALUE) {
                if(frame->pkt_dts != AV_NOPTS_VALUE) frame->pts = frame->pkt_dts;
                else if(prevPts != frame->pts) frame->pkt_dts = guestPts();
            }

            if(frame->pts != AV_NOPTS_VALUE) {
                if(prevPts >= 0){
                    int64_t f = frame->pts - prevPts;
                    if(f > 0 && avgFps < 0) avgFps = f;
                    else if(f > 0) avgFps = qMin(f, avgFps);
                }

                prevPts = frame->pts;
            }

            if(frame->pts == AV_NOPTS_VALUE) {
                if(frame->pkt_dts != AV_NOPTS_VALUE) frame->pts = frame->pkt_dts;
                else if(prevPts != frame->pts) frame->pkt_dts = guestPts();
            }

            int64_t timestamp = (frame->pts == AV_NOPTS_VALUE ? AV_NOPTS_VALUE : av_rescale_q(frame->pts, timeBase, AV_TIME_BASE_Q));
            if(frame->format == pixFormat) {
                av_frame_unref(hwFrame);
                if(av_hwframe_transfer_data(hwFrame, frame, 0) < 0){
                    qWarning() << "[MediaCodecDecoder::decode] 硬件解码失败";
                    break;
                }

                av_frame_unref(frame);
                av_frame_ref(frame, hwFrame);
            }

            emit sendFrame(timestamp, frame);
            frames.append(av_frame_clone(frame));
            ++ret;
        }
    }while(0);

    av_packet_unref(packet);
    av_packet_free(&packet);

    return ret;
}

void MediaCodecDecoder::initVideoDecoder(const MediaStream& stream)
{
    const AVCodec* codec = avcodec_find_decoder(stream.stream->codecpar->codec_id);

    if(codec == nullptr) {
        qWarning() << "[MediaCodecDecoder::initVideoDecoder] 无法找到" << avcodec_get_name(stream.stream->codecpar->codec_id) << "格式";
        return;
    }else {
        qDebug() << "[MediaCodecDecoder::initVideoDecoder] 申请" << codec->name << "格式";;
    }

    // 视频初始化
    int i = -1;
    do {
        ++i;
        // 1. 找到视频流下标

        // 2. 查询硬件加速
        AVHWDeviceType type = AV_HWDEVICE_TYPE_D3D11VA;
        for (;; i++) {
            const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
            if (!config) {
                qInfo() << "[MediaCodecDecoder::initVideoDecoder] 不支持硬件加速";
                type = AV_HWDEVICE_TYPE_NONE;
                break;
            }

            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && type == config->device_type) {
                pixFormat = config->pix_fmt;
                qDebug() << "[MediaCodecDecoder::initVideoDecoder] 匹配硬件解码 " << av_hwdevice_get_type_name(type);
                break;
            }
//            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) {
//                type = config->device_type;
//                pixFormat = config->pix_fmt;
//                qDebug() << "[MediaCodecDecoder::initVideoDecoder] 匹配硬件解码 " << av_hwdevice_get_type_name(type);
//                break;
//            }
        }

        // 3. 根据视频流格式获取对应格式的解码器
        ctx = avcodec_alloc_context3(codec);
        if(ctx == nullptr) {
            qWarning() << "[MediaCodecDecoder::initVideoDecoder] 申请视频解码器失败";
            break;
        }

        if(type != AV_HWDEVICE_TYPE_NONE){
            pixFormats[ctx] = pixFormat;

            // 4. 从视频流中将对应的参数拷贝给新的解码器
            if(avcodec_parameters_to_context(ctx, stream.stream->codecpar) < 0) {
                qWarning() << "[MediaCodecDecoder::initVideoDecoder] 获取视频解码器参数失败";
                continue;
            }

            // 5. 加载硬件加速器

            ctx->get_format = [](AVCodecContext *ctx, const AVPixelFormat *pix_fmts)->AVPixelFormat{
                const AVPixelFormat *p;

                for (p = pix_fmts; *p != -1; p++) {
                    if (*p == pixFormats[ctx]){
                        return *p;
                    }
                }

                return AV_PIX_FMT_NONE;
            };

            // 6. 创建硬件解码器
            if (av_hwdevice_ctx_create(&buffer, type, nullptr, nullptr, 0) < 0) {
                qWarning() << "[MediaCodecDecoder::initVideoDecoder] 申请硬件解码器失败";
                type = AV_HWDEVICE_TYPE_NONE;
                pixFormat = AV_PIX_FMT_NONE;
                continue;
            }

            if(type != AV_HWDEVICE_TYPE_NONE) {
                ctx->hw_device_ctx = av_buffer_ref(buffer);
                qInfo() << "[MediaCodecDecoder::initVideoDecoder] 选择硬件加速" << av_hwdevice_get_type_name(type);
            }else {
                continue;
            }
        }else {
            if(avcodec_parameters_to_context(ctx, stream.stream->codecpar) < 0) {
                qWarning() << "[MediaCodecDecoder::initVideoDecoder] 获取视频解码器参数失败";
            }
        }

        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        // 6. 打开解码器
        if(avcodec_open2(ctx, codec, nullptr) < 0) {
            qWarning() << "[MediaCodecDecoder::initVideoDecoder] 打开视频解码器失败";
            break;
        }

        qDebug() << ctx->codec_type << ' ' << ctx->codec_id << ' '
                 << ctx->pix_fmt << ' '
                 << ctx->width << ' ' << ctx->height;
        break;
    }while(1);
}

void MediaCodecDecoder::initAudioDecoder(const MediaStream& stream)
{
    // 1. 找到音频流对应的格式
    const AVCodec* codec = avcodec_find_decoder(stream.stream->codecpar->codec_id);
    if(codec == nullptr) {
        qWarning() << "[MediaCodecDecoder::initAudioDecoder] 无法找到" << avcodec_get_name(stream.stream->codecpar->codec_id) << "格式";
        return;
    }else {
        qDebug() << "[MediaCodecDecoder::initAudioDecoder] 申请" << codec->name << "格式";;
    }

    // 音频初始化
    do {

        // 4. 根据音频流格式获取对应格式的解码器
        ctx = avcodec_alloc_context3(codec);
        if(ctx == nullptr) {
            qWarning() << "[MediaCodecDecoder::initAudioDecoder] 申请音频解码器失败";
            break;
        }

        // 5. 从音频流中将对应的参数拷贝给新的解码器
        if(avcodec_parameters_to_context(ctx, stream.stream->codecpar) < 0) {
            qWarning() << "[MediaCodecDecoder::initAudioDecoder] 获取音频解码器参数失败";
            break;
        }

        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        // 6. 打开解码器
        if(avcodec_open2(ctx, codec, nullptr) < 0) {
            qWarning() << "[MediaCodecDecoder::initAudioDecoder] 打开音频解码器失败";
            break;
        }

        qDebug() << ctx->codec_type << ' ' << ctx->codec_id << ' '
                 << ctx->sample_fmt << ' '
                 << ctx->channels << ' ' << ctx->channel_layout << ' '
                 << ctx->sample_rate;
    }while(0);
}

int64_t MediaCodecDecoder::guestPts()
{
    int64_t ret = AV_NOPTS_VALUE;
    if(prevPts >= 0 && avgFps >= 0) ret = prevPts + avgFps;

    return ret;
}

/** Media Encoder **/
MediaCodecEncoder::MediaCodecEncoder(const AVCodecParameters& par, AVRational timeBase, AVRational streamTimebase, QObject* parent)
    : QObject(parent), ctx(nullptr), packet(nullptr), startTime(-1), streamTimebase(streamTimebase)
{
    switch(par.codec_type){
    case AVMEDIA_TYPE_VIDEO:
        initVideoEncoder(par, timeBase);
        break;
    case AVMEDIA_TYPE_AUDIO:
        initAudioEncoder(par, timeBase);
        break;
    default:
        break;
    }

    packet = av_packet_alloc();
}

MediaCodecEncoder::~MediaCodecEncoder()
{
    if(packet) {
        av_packet_unref(packet);
        av_packet_free(&packet);
    }

    if(ctx) avcodec_free_context(&ctx);
}

int MediaCodecEncoder::encode(AVFrame* frame, int64_t timestamp)
{
    if(ctx == nullptr) return false;

    int pts = 0;
    if(startTime < 0)  startTime = timestamp;
    else pts = av_rescale_q(timestamp - startTime, AV_TIME_BASE_Q, streamTimebase);

    frame->pts = pts;

    if(avcodec_send_frame(ctx, frame) < 0) {
        qWarning() << "[MediaCodecEncoder::encode] frame无法发送解码";
        return -1;
    }

    int ret = 0;
    while(1) {
        av_packet_unref(packet);

        int ret = 0;
        if((ret = avcodec_receive_packet(ctx, packet)) < 0){
            ret = false;
            break;
        }

        emit sendPacket(packet);
        ++ret;
    }

    av_frame_unref(frame);
    av_frame_free(&frame);

    return ret;
}

int MediaCodecEncoder::encode(AVFrame* frame, int64_t timestamp, QList<AVPacket*>& packets)
{
    if(ctx == nullptr || frame == nullptr) return -1;

    int pts = 0;
    if(startTime < 0)  startTime = timestamp;
    else pts = av_rescale_q(timestamp - startTime, AV_TIME_BASE_Q, streamTimebase);

    frame->pts = frame->pkt_dts = pts;

    if(avcodec_send_frame(ctx, frame) < 0) {
        qWarning() << "[MediaCodecEncoder::encode] frame无法发送解码";
        return -1;
    }

    while(1) {
        av_packet_unref(packet);

        if(avcodec_receive_packet(ctx, packet) < 0){
            break;
        }

        packets.append(av_packet_clone(packet));
        emit sendPacket(packet);
    }

    av_frame_unref(frame);
    av_frame_free(&frame);

    return packets.size();
}

void MediaCodecEncoder::initVideoEncoder(const AVCodecParameters& par, AVRational timeBase)
{
    const AVCodec* codec = avcodec_find_encoder(par.codec_id);
//    if(codec->id == AV_CODEC_ID_H264) codec = avcodec_find_encoder_by_name("h264_amf");

    if(codec == nullptr) {
        qWarning() << "[MediaCodecEncoder::initVideoEncoder] 无法找到" << avcodec_get_name(par.codec_id) << "格式";
        return;
    }else {
        qDebug() << "[MediaCodecEncoder::initVideoEncoder] 申请" << codec->name << "格式";;
    }

    ctx = avcodec_alloc_context3(codec);
    if(ctx == nullptr) {
        qWarning() << "[MediaCodecEncoder::initVideoEncoder] 申请视频转码器失败";
        return;
    }

    ctx->codec_id = codec->id;
    ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    ctx->time_base = timeBase;
    ctx->pix_fmt = *(codec->pix_fmts);  // 0
    ctx->width = par.width;
    ctx->height = par.height;

    AVDictionary* dict = nullptr;
    av_dict_set(&dict, "preset", "slow", 0);
    av_dict_set(&dict, "crf", "40", 0);
//    av_dict_set(&dict, "rc", "3", 0);
    av_dict_set(&dict, "rtbufsize", "40M", 0);

    ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    if(avcodec_open2(ctx, codec, &dict) < 0) {
        qWarning() << "[MediaCodecEncoder::initVideoEncoder] 申请视频转码器失败";
        avcodec_free_context(&ctx);
    }

    av_dict_free(&dict);
}

void MediaCodecEncoder::initAudioEncoder(const AVCodecParameters& par, AVRational timeBase)
{
    const AVCodec* codec = avcodec_find_encoder(par.codec_id);
    if(codec == nullptr) {
        qWarning() << "[MediaCodecEncoder::initAudioEncoder] 无法找到" << avcodec_get_name(par.codec_id) << "格式";
        return;
    }else {
        qDebug() << "[MediaCodecEncoder::initAudioEncoder] 申请" << codec->name << "格式";
    }

    ctx = avcodec_alloc_context3(codec);
    if(ctx == nullptr) {
        qWarning() << "[MediaCodecEncoder::initAudioEncoder] 申请音频转码器失败";
        return;
    }

    ctx->codec_id = codec->id;
    ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    ctx->time_base = timeBase;
    ctx->sample_fmt = *(codec->sample_fmts);            // 7
    ctx->channel_layout = *(codec->channel_layouts);    // 4
    ctx->channels = av_get_channel_layout_nb_channels(ctx->channel_layout);
    ctx->sample_rate = *(codec->supported_samplerates); // 44100

    AVDictionary* dict = nullptr;
    av_dict_set(&dict, "rtbufsize", "4194304", 0);

    ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if(avcodec_open2(ctx, codec, &dict) < 0) {
        qWarning() << "申请音频解码器失败";
        avcodec_free_context(&ctx);
        return;
    }

    av_dict_free(&dict);
}

