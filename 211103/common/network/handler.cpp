#include "handler.h"

Handler::Handler(QTcpSocket* socket, Packer* packer)
    : s(socket), p(packer)
{

}

Handler::~Handler()
{

}

void Handler::reset()
{

}
