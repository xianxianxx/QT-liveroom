#ifndef HANDLER_H
#define HANDLER_H

#include <QTcpSocket>

#include "packer.h"

class Packet;

class Handler
{
public:
    Handler(QTcpSocket* socket, Packer* packer);
    virtual ~Handler();

    virtual void handle(const Packet& packet)=0;
    virtual void reset();

    inline QTcpSocket* socket() { return s; }
    inline Packer* packer() { return p; }

private:
    QTcpSocket* s;
    Packer* p;
};

#endif // HANDLER_H
