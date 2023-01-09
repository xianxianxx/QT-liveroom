#ifndef MEDIAIODEVICE_H
#define MEDIAIODEVICE_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QEventLoop>
#include <QMutex>
#include <QWaitCondition>
#include <QEventLoop>

class MediaIODevice : public QObject
{
    Q_OBJECT
public:
    MediaIODevice(QObject* parent = nullptr);
    ~MediaIODevice();

    void open();
    void close();

    inline bool isOpen() const{ return writer != nullptr && reader != nullptr; }

    inline qint64 write(const QByteArray& array){ return write(array.data(), array.size()); }
    bool waitForReadyRead(int msec = 30);

    qint64 write(const char* data, int len);

    qint64 read(char* data, qint64 len);
    QByteArray read(qint64 maxSize);
    QByteArray readAll();

private slots:
    void onNewConnection();
    qint64 writePrivate(const char* data, qint64 len);
    qint64 readPrivate(char* data, qint64 len);
    void releaseReadyRead();

signals:
    void sigWrite(const char* data, qint64 len);
    void readyRead();
    void sigRead(char* data, qint64 len);

private:
    QTcpServer* server;
    QTcpSocket* writer;
    QTcpSocket* reader;

    QEventLoop* loop;

    qint64 bytesWritten;
    qint64 bytesReaded;

    int port;

    QMutex mutex;
    QWaitCondition cond;
};

#endif // MEDIAIODEVICE_H
