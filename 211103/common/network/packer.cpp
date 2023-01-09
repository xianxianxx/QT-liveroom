#include "packer.h"

#include <QJsonDocument>
#include <QJsonObject>

Packer::Packer()
{

}

QByteArray Packer::pack(const Packet& packet)
{
    /** 定长 + 变长 **/
    QByteArray head(8, 0), body;

    QJsonObject object = QJsonObject::fromVariantMap(packet.getMap());
    QJsonDocument document = QJsonDocument(object);

    body = document.toJson(QJsonDocument::Compact);

    int len = body.size(), type = packet.type;
    memcpy(head.data(), &len, 4);
    memcpy(head.data() + 4, &type, 4);

    return head + body;
}

int Packer::unpack(const QByteArray& buffer, Packet& packet)
{
    if(buffer.size() < 8) return 0;

    int len = 0, type = 0;
    memcpy(&len, buffer.data(), 4);
    memcpy(&type, buffer.data() + 4, 4);

    if(buffer.size() < (8 + len)) return 0;

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(buffer.mid(8, len), &error);

    if(error.error != QJsonParseError::NoError) return -1;

    QJsonObject object = document.object();
    QVariantMap map = object.toVariantMap();

    packet.type = (Packet::Type)type;
    packet.map = std::move(map);

    return 8 + len;
}
