#include "mediatimer.h"
#include "mediaprocessor.h"

extern "C"
{
#include "libavutil/time.h"
}

#include <QDebug>

MediaTimer::MediaTimer(QObject* parent)
    : QThread(parent), startTime(-1), prevTime(-1), delay(0)
{
}

MediaTimer::MediaTimer(int64_t startTime, QObject* parent)
    : QThread(parent), startTime(-1), prevTime(startTime), delay(0)
{
}

MediaTimer::~MediaTimer()
{
    stop();
    queue.clear();
}

bool MediaTimer::start(int64_t delay)
{
    if(isRunning()) {
        qWarning() << "[MediaTimer::start] 正在运行，无法开启";
        return false;
    }

    if(prevTime < 0) {
        qWarning() << "[MediaTimer::start] 未设置开始时间";
        return false;
    }

    queue.clear();

    this->delay = delay;
    QThread::start();

    return true;
}

void MediaTimer::stop()
{
    if(isRunning() == false) return;

    threadExitFlag = false;

    wait();
}

void MediaTimer::enqueueFrame(int64_t timestamp, AVFrame* frame)
{
    queue.enqueue(timestamp, av_frame_clone(frame));
}

void MediaTimer::run()
{
    qDebug() << "[MediaTimer::run()] 定时器开启";

    QThread::usleep(delay);

    startTime = av_gettime();
    threadExitFlag = true;
    while(threadExitFlag) {
        if(queue.empty()){
            emit needFrames();
            sem.tryAcquire(1, 30);
        }

        if(threadExitFlag == false) break;
        if(queue.empty()) continue;

        int64_t timestamp = queue.nextFrameTimestamp();
        if(timestamp == AV_NOPTS_VALUE) {
            queue.dequeue();
            continue;
        }

        int64_t nextTime = timestamp - prevTime;

        if(nextTime < 0) continue;

        int64_t currentTime = av_gettime();

        int64_t slTime = nextTime - (currentTime - startTime);
//        qDebug() << slTime;
        if(slTime > 0) QThread::usleep(slTime);

        if(queue.empty()) continue;
        if(threadExitFlag == false) break;

        AVFrame* frame = queue.dequeue().second;
        if(threadExitFlag == false) break;

        emit sendFrame(frame);
        av_frame_unref(frame);
        av_frame_free(&frame);
    }

    delay = 0;
    qDebug() << "[MediaTimer::stop] 定时器结束";
}
