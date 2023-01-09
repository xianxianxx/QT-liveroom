#ifndef MEDIASTREAM_H
#define MEDIASTREAM_H

extern "C"
{
#include "libavformat/avformat.h"
}

class MediaStream{
friend class MediaProcessor;
friend class MediaCodecDecoder;
friend class MediaCodecEncoder;
friend class MediaPlayer;
friend class MediaPipe;

public:
    MediaStream(int idx = -1, AVStream* stream = nullptr);
    inline AVMediaType getMediaType() const{ return stream == nullptr ? AVMEDIA_TYPE_UNKNOWN : stream->codecpar->codec_type; }

private:
    int idx;
    AVStream* stream;
};

#endif // MEDIASTREAM_H
