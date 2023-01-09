#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include <QObject>
#include <QTcpSocket>

#include "singleton.h"
#include "handlecontext.h"
#include "packet.h"
#include "handlers/clientuserhandler.h"
#include "handlers/clientroomhandler.h"

class ClientSocket : public QObject
{
friend class SingleTon<ClientSocket>;

    Q_OBJECT
private:
    explicit ClientSocket(QObject *parent = nullptr);
    ClientSocket(const ClientSocket&)=delete;
    ClientSocket(ClientSocket&&)=delete;
    ClientSocket& operator=(const ClientSocket&)=delete;
    ClientSocket& operator=(ClientSocket&&)=delete;

public:
    inline ClientUserHandler* userHander(){ return static_cast<ClientUserHandler*>(ctx->getHandler((int)Packet::user)); }
    inline ClientRoomHandler* roomHander(){ return static_cast<ClientRoomHandler*>(ctx->getHandler((int)Packet::room)); }

    inline qint64 write(const QByteArray& stream){ return socket->write(stream); }
    bool connectToHost(const QString& addr, int port);

private slots:
    void onReadyRead();

signals:

private:
    QTcpSocket* socket;

    HandleContext* ctx;
};

#endif // CLIENTSOCKET_H
