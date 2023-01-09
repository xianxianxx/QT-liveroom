#ifndef MEDIATIMER_H
#define MEDIATIMER_H

#include <QThread>
#include <QSemaphore>

#include "mediaqueue.h"

class MediaTimer : public QThread
{
friend class MediaProcessor;

    Q_OBJECT
public:
    MediaTimer(QObject* parent = nullptr);
    MediaTimer(int64_t startTime, QObject* parent = nullptr);

    virtual ~MediaTimer();

    inline void setStartTime(int64_t startTime){ prevTime = startTime; }
    inline void release() { if(sem.available() == 0) sem.release(); }
    bool start(int64_t delay = 0);
    void stop();

public slots:
    void enqueueFrame(int64_t timestamp, AVFrame* frame);

signals:
    void sendFrame(AVFrame* frame);
    void needFrames();

protected:
    virtual void run() override;

private:
    MediaQueue queue;

    int64_t startTime;
    int64_t prevTime;
    bool threadExitFlag;

    int64_t delay;
    QSemaphore sem;
};

#endif // MEDIATIMER_H
