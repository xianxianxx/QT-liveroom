#ifndef CLIENTUSERHANDLER_H
#define CLIENTUSERHANDLER_H

#include "handler.h"

class ClientUserHandler : public Handler
{
public:
    ClientUserHandler(QTcpSocket* socket, Packer* packer);
    virtual void handle(const Packet& packet) override;

    void regist(const QString& username, const QString& password);
    void login(const QString& username, const QString& password);
    void charge(const QString& username, int money);
    void gift(const QString& from, const QString& to, const QString& name, int money);

private:
    void login(const Packet& packet);
    void regist(const Packet& packet);
    void force(const Packet& packet);
    void charge(const Packet& packet);
    void gift(const Packet& packet);
};

#endif // CLIENTUSERHANDLER_H
