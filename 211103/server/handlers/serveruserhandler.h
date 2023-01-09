#ifndef SERVERUSERHANDLER_H
#define SERVERUSERHANDLER_H

#include <QMap>

#include "handler.h"
#include "serversocket.h"

class ServerUserHandler : public Handler
{
private:
    struct UserContainer{
        QMap<QString, ServerSocket*> users;
    };

public:
    ServerUserHandler(ServerSocket* socket, Packer* packer);
    virtual void handle(const Packet& packet) override;
    virtual void reset() override;

private:
    void regist(const Packet& packet);
    void login(const Packet& packet);
    void charge(const Packet& packet);
    void gift(const Packet& packet);


    ServerSocket* socket;
    QString username;

    static UserContainer userContainer;
};

#endif // SERVERUSERHANDLER_H
