#ifndef PACKET_H
#define PACKET_H

#include <QVariantMap>

/**
 * @class Packet
 * @brief 管理套接字发收数据时的应用层协议包
 * @details
 *  记录类型，以及根据用户的需求可以以key-value形式
 *  存放需要的类型，类似于json
 */
class Packet
{
friend class Packer;

public:
    enum Type{
        none = -1,
        user,
        room,
        biggest
    };

    Packet(Type type = none);

    inline Type getType() const{ return this->type; }

    const QVariantMap& getMap() const{ return map; }

    QVariant getValue(const QString& key) const;
    void setValue(const QString& key, const QVariant& var);

private:
    Type type;

    QVariantMap map;
};

#endif // PACKET_H
