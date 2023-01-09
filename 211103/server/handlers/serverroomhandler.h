#ifndef SERVERROOMHANDLER_H
#define SERVERROOMHANDLER_H

#include "handler.h"
#include <queue>
#include <QSet>

class ServerSocket;

class ServerRoomHandler : public Handler
{
private:
    struct User{
        QString username;
        ServerSocket* socket;
    };

    struct Room{
        int id;
        QString username;
        QString roomname;
        QString password;
        int limit;

        QMap<QString, User> users;
    };

    struct RoomIdGenerator{
        int id;

        RoomIdGenerator() : id(0) {}
        std::priority_queue<int, std::vector<int>, std::greater<int>> pq;
        inline int getId(){
            if(pq.empty()) return id++;
            int ret = pq.top();
            pq.pop();
            return ret;
        }

        inline void push(int id) { pq.push(id); }
    };

public:
    ServerRoomHandler(ServerSocket* socket, Packer* packer);
    virtual void handle(const Packet& packet) override;

    virtual void reset() override;

private:
    void create(const Packet& packet);
    void reloadRoom(const Packet& packet);
    void inHall(const Packet& packet);
    void quitRoom(const Packet& packet);
    void intoRoom(const Packet& packet);
    void reloadUser(const Packet& packet);
    void chat(const Packet& packet);
    void message(const Packet& packet);
    void video(const Packet& packet);
    void audio(const Packet& packet);

    void notifyNewRoom(const Room& room);
    void notifyDeleteRoom(const Room& room);
    void notifyModifyRoom(const Room& room);
    void notifyUser(const Room& room, const QString& username, bool add);

    static QMap<int, Room> rooms;
    static QSet<ServerSocket*> inHalls;
    static RoomIdGenerator generator;

    int currentRoomid; // 我自己创建的房间
    QList<std::pair<int, QString>> currentRoom;

    ServerSocket* socket;
};

#endif // SERVERROOMHANDLER_H
