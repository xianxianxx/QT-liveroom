#ifndef PACKER_H
#define PACKER_H

#include <QByteArray>

#include "packet.h"

/**
 * @class Packer
 * @brief 负责stream转换为Packet(拆包)，以及Packet转换为stream(打包)
 * @details 定长+变长
 *  定长 type + len (8bytes)
 *  变长 json
 */
class Packer
{
public:
    Packer();

    QByteArray pack(const Packet& packet);
    int unpack(const QByteArray& buffer, Packet& packet);
};

#endif // PACKER_H
