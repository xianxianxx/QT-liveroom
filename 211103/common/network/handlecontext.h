#ifndef HANDLECONTEXT_H
#define HANDLECONTEXT_H

#include <QByteArray>
#include <QVector>

class Packer;
class Handler;
class Packet;

/**
 * @class HandleContext
 * @brief 负责数据流缓存以及处理功能
 * @details
 *  当服务器/客户端的套接字收到数据之后，载入此对象的缓存中，
 *  并负责从该缓存拆包，拆出新的包之后，根据包的类型调用不同的策略进行不从的调度
 */
class HandleContext
{
public:
    HandleContext(Packer* packer, QVector<Handler*> handlers);
    virtual ~HandleContext();

    inline void append(const QByteArray& stream){ buffer += stream; }

    void handle();
    inline void handle(const QByteArray& stream){
        append(stream);
        handle();
    }

    void handle(const Packet& packet);
    Handler* getHandler(int idx);
    void reset();

private:
    QByteArray buffer;
    Packer* packer;

    QVector<Handler*> handlers;
};

#endif // HANDLECONTEXT_H
