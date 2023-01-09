#include "clientsocket.h"

ClientSocket::ClientSocket(QObject *parent)
    : QObject(parent)
{
    socket = new QTcpSocket(this);

    QObject::connect(socket, SIGNAL(readyRead()),
                     this, SLOT(onReadyRead()));

    QVector<Handler*> handlers((int)Packet::biggest);

    Packer* packer = new Packer;

    // 手动添加
    handlers[Packet::user] = new ClientUserHandler(socket, packer);
    handlers[Packet::room] = new ClientRoomHandler(socket, packer);

    ctx = new HandleContext(packer, handlers);
}

bool ClientSocket::connectToHost(const QString& addr, int port)
{
    socket->connectToHost(addr, port);
    return socket->waitForConnected(1000);
}

void ClientSocket::onReadyRead()
{
    // 新的数据
    QByteArray stream = socket->readAll();

    ctx->handle(stream);
}
