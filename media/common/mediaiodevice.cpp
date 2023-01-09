#include "mediaiodevice.h"

#include <QThread>
#include <QHostAddress>
#include <QCoreApplication>

MediaIODevice::MediaIODevice(QObject* parent)
    : QObject(parent), writer(nullptr), reader(nullptr), loop(new QEventLoop(this))
{
    server = new QTcpServer(this);

    QObject::connect(this, &MediaIODevice::sigWrite, this, &MediaIODevice::writePrivate, Qt::BlockingQueuedConnection);
    QObject::connect(this, &MediaIODevice::sigRead, this, &MediaIODevice::readPrivate, Qt::BlockingQueuedConnection);
    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [=](){
            close();
    });

    do{
        port = qrand() % 10000 + 10000;
    }while(server->listen(QHostAddress::LocalHost, port) == false);
}

MediaIODevice::~MediaIODevice()
{
    close();
}

void MediaIODevice::open()
{
    if(isOpen()) return;

    QObject::connect(server, &QTcpServer::newConnection, this, &MediaIODevice::onNewConnection);

    writer = new QTcpSocket(this);
    writer->connectToHost(QHostAddress::LocalHost, port);
    if(writer->waitForConnected(100) == false) {
        qWarning() << "[MediaIODevice::open] 打开IOdevice失败";
        writer->deleteLater();
        writer = nullptr;

        QObject::disconnect(server, &QTcpServer::newConnection, this, &MediaIODevice::onNewConnection);
    }
}

void MediaIODevice::close()
{
    if(writer) {
        writer->deleteLater();
        writer = nullptr;
    }

    if(reader) {
        reader->deleteLater();
        reader = nullptr;
    }

    cond.wakeAll();
}

bool MediaIODevice::waitForReadyRead(int msec)
{
    if(reader == nullptr) return false;

    mutex.lock();
    bool ret = cond.wait(&mutex, msec);
    mutex.unlock();

    return ret;
}

qint64 MediaIODevice::write(const char* data, int len)
{
    if(writer == nullptr) return -1;

    if(parent()->thread() == QThread::currentThread()) return writePrivate(data, len);

    emit sigWrite(data, len);
    return bytesWritten;
}

qint64 MediaIODevice::read(char* data, qint64 len)
{
    if(reader == nullptr) return -1;
    if(parent()->thread() == QThread::currentThread()) return readPrivate(data, len);

    emit sigRead(data, len);
    return bytesReaded;
}

QByteArray MediaIODevice::read(qint64 maxSize)
{
    if(reader == nullptr) return {};

    maxSize = qMin(maxSize, reader->bytesAvailable());
    if(maxSize <= 0) return {};

    QByteArray array(maxSize, 0);
    int ret = read(array.data(), array.size());
    if(ret < array.size()) array.resize(ret);
    return array;
}

QByteArray MediaIODevice::readAll()
{
    if(reader == nullptr) return {};

    return read(reader->bytesAvailable());
}

void MediaIODevice::onNewConnection()
{
    if(isOpen()) return;

    QObject::disconnect(server, &QTcpServer::newConnection, this, &MediaIODevice::onNewConnection);
    while(server->hasPendingConnections()){
        reader = server->nextPendingConnection();
        if(reader != nullptr) break;
    }

    if(reader == nullptr) return;
    QObject::connect(reader, &QTcpSocket::readyRead, this, &MediaIODevice::readyRead);
    QObject::connect(reader, &QTcpSocket::readyRead, this, &MediaIODevice::releaseReadyRead);
}

qint64 MediaIODevice::writePrivate(const char* data, qint64 len)
{
    if(writer == nullptr) return -1;
    bytesWritten = -1;
    bytesWritten = writer->write(data, len);

    return bytesWritten;
}

qint64 MediaIODevice::readPrivate(char* data, qint64 len)
{
    if(reader == nullptr) return -1;
    bytesReaded = -1;
    bytesReaded = reader->read(data, len);

    return bytesReaded;
}

void MediaIODevice::releaseReadyRead()
{
    cond.wakeAll();
}
