#include "mediaqueue.h"

#include <QDebug>

MediaQueue::MediaQueue()
{

}

MediaQueue::~MediaQueue()
{
    clear();
}

void MediaQueue::enqueue(int64_t timestamp, AVFrame* frame)
{
    mutex.lock();
    queue.enqueue(std::make_pair(timestamp, frame));
    mutex.unlock();
}

std::pair<int64_t, AVFrame*> MediaQueue::dequeue()
{
    std::pair<int64_t, AVFrame*> frame;

    mutex.lock();
    if(!queue.empty()) frame = queue.dequeue();
    mutex.unlock();

    return frame;
}

void MediaQueue::clear()
{
    mutex.lock();
    while(!queue.isEmpty()) {
        AVFrame* frame = queue.dequeue().second;
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
    mutex.unlock();
}

qint64 MediaQueue::nextFrameTimestamp() const
{
    return queue.isEmpty() ? -1 : queue.front().first;
}
