#include "packet.h"

Packet::Packet(Packet::Type type)
    : type(type)
{

}

QVariant Packet::getValue(const QString& key) const
{
    auto it = map.find(key);
    if(it == map.end()) return {};

    return it.value();
}

void Packet::setValue(const QString& key, const QVariant& var)
{
    map[key] = var;
}
