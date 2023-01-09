#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <QObject>
#include <QTcpSocket>

#include "handlecontext.h"

class ServerSocket : public QObject
{
    Q_OBJECT
public:
    explicit ServerSocket(QTcpSocket* socket, QObject *parent = nullptr);
    virtual ~ServerSocket();

    inline qint64 write(const QByteArray& stream){ return socket->write(stream); }
    qint64 write(const Packet& packet);

    QTcpSocket* getSocket() const{ return socket; }
    HandleContext* getContext() const{ return ctx; }

private slots:
    void onReadyRead();
    void onDisconnected();

signals:

private:
    QTcpSocket* socket;
    Packer* packer;
    HandleContext* ctx;
};

#endif // SERVERSOCKET_H
