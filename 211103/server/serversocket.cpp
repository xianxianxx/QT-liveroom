#include "serversocket.h"
#include "packet.h"
#include "packer.h"
#include "handlers/serveruserhandler.h"
#include "handlers/serverroomhandler.h"
#include <QDebug>
#include <QHostAddress>

ServerSocket::ServerSocket(QTcpSocket* socket, QObject *parent)
    : QObject(parent), socket(socket)
{
    qDebug() << "[ServerSocket::ServerSocket] 新的连接: <地址>"
             << socket->peerAddress().toString() << " <端口>"
             << socket->peerPort();

    QObject::connect(socket, SIGNAL(readyRead()),
                     this, SLOT(onReadyRead()));

    QObject::connect(socket, SIGNAL(disconnected()),
                     this, SLOT(onDisconnected()));

    packer = new Packer;

    // 手动添加
    QVector<Handler*> handlers((int)Packet::biggest);
    handlers[(int)Packet::user] = new ServerUserHandler(this, packer);
    handlers[(int)Packet::room] = new ServerRoomHandler(this, packer);

    ctx = new HandleContext(packer, handlers);
}

ServerSocket::~ServerSocket()
{
    delete ctx;
    delete packer;
}

qint64 ServerSocket::write(const Packet& packet)
{
    return socket->write(packer->pack(packet));
}

void ServerSocket::onReadyRead()
{
    // 新的数据
    QByteArray stream = socket->readAll();

    ctx->handle(stream);
}

void ServerSocket::onDisconnected()
{
    qDebug() << "[ServerSocket::ServerSocket] 断开连接: <地址>"
             << socket->peerAddress().toString() << " <端口>"
             << socket->peerPort();

    ctx->reset();
    socket->deleteLater();
    this->deleteLater();
}
