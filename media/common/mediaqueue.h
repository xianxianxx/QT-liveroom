#ifndef MEDIAQUEUE_H
#define MEDIAQUEUE_H

extern "C"
{
#include "libavformat/avformat.h"
}

#include <QQueue>
#include <QMutex>

class MediaQueue
{
friend class MediaProcessor;

public:
    MediaQueue();
    ~MediaQueue();

    void enqueue(int64_t timestamp, AVFrame* frame);
    std::pair<int64_t, AVFrame*> dequeue();

    void clear();
    qint64 nextFrameTimestamp() const;

    inline int size() const{return queue.size(); }
    inline bool empty() const{ return queue.isEmpty(); }

private:
    QQueue<std::pair<qint64, AVFrame*>> queue;
    QMutex mutex;
};

#endif // MEDIAQUEUE_H
