#ifndef CLIENTROOMHANDLER_H
#define CLIENTROOMHANDLER_H

#include "handler.h"
#include <QAudioBuffer>

class ClientRoomHandler : public Handler
{
public:
    ClientRoomHandler(QTcpSocket* socket, Packer* packer);
    virtual void handle(const Packet& packet) override;

    void create(const QString& username, const QString& roomname,
                const QString& password, int limit);
    void reloadRoom();
    void inHall(bool in);
    void quitRoom(int roomid, const QString& username);
    void intoRoom(int roomid, const QString& username, const QString& password);
    void reloadUser(int roomid);
    void chat(int roomid, const QString& username, const QString& message);
    void message(int roomid, const QString& message);
    void video(int roomid, const QImage& image);
    void audio(int roomid,  const QByteArray& audio);

private:
    void create(const Packet& packet);
    void reloadRoom(const Packet& packet);
    void notify(const Packet& packet);
    void quitRoom(const Packet& packet);
    void intoRoom(const Packet& packet);
    void reloadUser(const Packet& packet);
    void notifyUser(const Packet& packet);
    void chat(const Packet& packet);
    void message(const Packet& packet);
    void video(const Packet& packet);
    void audio(const Packet& packet);
};

#endif // CLIENTROOMHANDLER_H
