#include "server.h"
#include "serversocket.h"

#include <QHostAddress>

Server::Server(QObject *parent)
    : QObject(parent)
{
    server = new QTcpServer(this);

    QObject::connect(server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

bool Server::listen(const QString& addr, int port)
{
    if(server->isListening()) return false;

    return server->listen(QHostAddress(addr), port);
}

void Server::onNewConnection()
{
    // 新的连接
    while(server->hasPendingConnections()) {
        QTcpSocket* socket = server->nextPendingConnection();
        if(socket) {
            new ServerSocket(socket, this);
        }
    }
}
