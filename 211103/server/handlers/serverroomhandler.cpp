#include "serverroomhandler.h"
#include "serversocket.h"

#include <QDateTime>

QMap<int, ServerRoomHandler::Room> ServerRoomHandler::rooms;
ServerRoomHandler::RoomIdGenerator ServerRoomHandler::generator;
QSet<ServerSocket*> ServerRoomHandler::inHalls;

ServerRoomHandler::ServerRoomHandler(ServerSocket* socket, Packer* packer)
    : Handler(socket->getSocket(), packer), socket(socket)
{
    currentRoomid = -1;
}

void ServerRoomHandler::handle(const Packet& packet)
{
    QString type = packet.getValue("type").toString();

    if(type == "create") {
        create(packet);
    }else if(type == "reloadRoom") {
        reloadRoom(packet);
    }else if(type == "inHall") {
        inHall(packet);
    }else if(type == "quitRoom") {
        quitRoom(packet);
    }else if(type == "intoRoom") {
        intoRoom(packet);
    }else if(type == "reloadUser") {
        reloadUser(packet);
    }else if(type == "chat") {
        chat(packet);
    }else if(type == "message") {
        message(packet);
    }else if(type == "video") {
        video(packet);
    }else if(type == "audio") {
        audio(packet);
    }
}

void ServerRoomHandler::reset()
{
    // 清理状态
    // 1. 清理大厅通知
    inHalls.remove(socket);

    // 2. 我在的所有房间删掉
    for(auto p : currentRoom) {
        int roomid = p.first;
        QString username = p.second;

        auto it = rooms.find(roomid);
        if(it == rooms.end()) continue;

        Room& room = it.value();

        auto it2 = room.users.find(username);
        if(it2 != room.users.end()){
            room.users.erase(it2);
            notifyModifyRoom(room);
            notifyUser(room, username, false);
        }
    }
    currentRoom.clear();

    // 3. 我自己的房间删除掉
    auto it = rooms.find(currentRoomid);
    if(it != rooms.end()) {
        // 删除自己的房间
        Room& room = it.value();

        Packet p(Packet::room);
        p.setValue("type", "quitRoom");
        p.setValue("operation", "force");

        for(auto& user : room.users) {
            user.socket->write(p);
        }

        currentRoomid = -1;
        generator.push(room.id);
        notifyDeleteRoom(room);

        rooms.erase(it);
    }
}

void ServerRoomHandler::create(const Packet& packet)
{
    QString username = packet.getValue("username").toString();
    QString roomname = packet.getValue("roomname").toString();
    QString password = packet.getValue("password").toString();
    int limit = packet.getValue("limit").toInt();

    Packet p(Packet::room);
    p.setValue("type", "create");

    do{
        // 1. 先判断是否重复创建房间
        if(this->currentRoomid >= 0) {
            // 失败，重复创建房间
            p.setValue("result", 1);
            p.setValue("info", "创建房间失败，已经创建一个房间");
            break;
        }

        // 2. 允许创建房间
        int roomid = generator.getId();
        while(rooms.count(roomid) > 0) {
            // roomid重复
            qDebug() << "[ServerRoomHandler::create] roomId重复";
            roomid = generator.getId();
        }

        Room room = {roomid, username, roomname, password, limit, QMap<QString, User>{{username, {username, socket}}}};
        auto& r = rooms[roomid];
        r = std::move(room);

        currentRoomid = roomid;

        p.setValue("result", 0);
        p.setValue("roomid", roomid);
        p.setValue("username", username);
        p.setValue("roomname", roomname);
        p.setValue("password", password);
        p.setValue("limit", limit);

        // 有新的房间被创建，需要通知用户新房间
        notifyNewRoom(r);
    }while(0);

    socket->write(p);
}

void ServerRoomHandler::reloadRoom(const Packet& packet)
{
    Q_UNUSED(packet);

    Packet p(Packet::room);
    p.setValue("type", "reloadRoom");

    QVariantList list;
    for(auto it = rooms.begin(); it != rooms.end(); ++it) {
        Room& room = it.value();

        QVariantMap roomMap;
        roomMap["roomid"] = room.id;
        roomMap["username"] = room.username;
        roomMap["roomname"] = room.roomname;
        roomMap["password"] = room.password;
        roomMap["limit"] = room.limit;
        roomMap["current"] = room.users.size();

        list.append(std::move(roomMap));
    }

    p.setValue("rooms", list);

    socket->write(p);
}

void ServerRoomHandler::inHall(const Packet& packet)
{
    bool in = packet.getValue("in").toBool();
    if(in) {
        // 要发
        inHalls.insert(socket);
    }else {
        // 不要发
        inHalls.remove(socket);
    }
}

void ServerRoomHandler::quitRoom(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();
    QString username = packet.getValue("username").toString();

    auto it = rooms.find(roomid);

    if(it == rooms.end()) return;

    Packet p(Packet::room);
    p.setValue("type", "quitRoom");

    Room& room = it.value();
    if(username == room.username) {
        // 房主退出
        p.setValue("operation", "force");

        for(auto& user : room.users) {
            user.socket->write(p);
        }

        currentRoomid = -1;
        generator.push(room.id);
        notifyDeleteRoom(room);
        rooms.erase(it);
    }else {
        // 普通用户
        for(auto it = currentRoom.begin(); it != currentRoom.end(); ++it) {
            if(it->first == roomid) {
                currentRoom.erase(it);
                break;
            }
        }

        auto it2 = room.users.find(username);
        if(it2 != room.users.end()){
            room.users.erase(it2);
            notifyModifyRoom(room);
            notifyUser(room, username, false);
        }

        socket->write(p);
    }
}

void ServerRoomHandler::intoRoom(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();
    QString username = packet.getValue("username").toString();
    QString password = packet.getValue("password").toString();

    Packet p(Packet::room);
    p.setValue("type", "intoRoom");

    do{
        // 1. 找房间
        auto it = rooms.find(roomid);
        if(it == rooms.end()) {
            p.setValue("result", 1);
            p.setValue("info", "进入房间失败，房间不存在");
            break;
        }

        Room& room = it.value();
        // 2. 判断房间密码
        if(password != room.password) {
            p.setValue("result", 1);
            p.setValue("info", "进入房间失败，密码错误");
            break;
        }

        // 3. 判断是否已经在房间中
        if(room.users.count(username) != 0) {
            // 已经在房间中
            p.setValue("result", 0);
            break;
        }

        // 4. 判断人数
        if(room.users.size() >= room.limit) {
            p.setValue("result", 1);
            p.setValue("info", "进入房间失败，人数已满");
            break;
        }

        // 添加房间用户
        currentRoom.append({roomid, username});
        room.users[username] = {username, socket};

        notifyModifyRoom(room);
        notifyUser(room, username, true);

        p.setValue("roomid", roomid);
        p.setValue("username", username);
        p.setValue("ownername", room.username);
        p.setValue("result", 0);
    }while(0);

    socket->write(p);
}

void ServerRoomHandler::reloadUser(const Packet& packet)
{
    // 1. 找到房间
    int roomid = packet.getValue("roomid").toInt();
    auto it = rooms.find(roomid);

    if(it == rooms.end()) return;
    Room& room = it.value();

    Packet p(Packet::room);
    p.setValue("type", "reloadUser");

    QVariantList list;
    for(auto& user : room.users) {
        list.append(user.username);
    }

    p.setValue("roomid", roomid);
    p.setValue("users", list);

    socket->write(p);
}

void ServerRoomHandler::chat(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();
    auto it = rooms.find(roomid);

    if(it == rooms.end()) return;

    Room& room = it.value();

    Packet p(Packet::room);
    p.setValue("type", "chat");
    p.setValue("roomid", roomid);

    QString username = packet.getValue("username").toString();
    QString message = packet.getValue("message").toString();
    QString time = QDateTime::currentDateTime().toString();

    p.setValue("username", username);
    p.setValue("message", message);
    p.setValue("datetime", time);

    for(auto& user : room.users) {
        user.socket->write(p);
    }
}

void ServerRoomHandler::message(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();
    auto it = rooms.find(roomid);

    if(it == rooms.end()) return;

    Room& room = it.value();

    Packet p(Packet::room);
    p.setValue("type", "message");
    p.setValue("roomid", roomid);
    p.setValue("message", packet.getValue("message").toString());

    for(auto& user : room.users) {
        user.socket->write(p);
    }
}

void ServerRoomHandler::video(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();
    auto it = rooms.find(roomid);

    if(it == rooms.end()) return;

    Room& room = it.value();

    for(auto& user : room.users) {
        user.socket->write(packet);
    }
}

void ServerRoomHandler::audio(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();
    auto it = rooms.find(roomid);

    if(it == rooms.end()) return;

    Room& room = it.value();

    for(auto& user : room.users) {
        user.socket->write(packet);
    }
}

void ServerRoomHandler::notifyNewRoom(const ServerRoomHandler::Room& room)
{
    Packet p(Packet::room);
    p.setValue("type", "notify");
    p.setValue("operation", "new");
    p.setValue("roomid", room.id);
    p.setValue("username", room.username);
    p.setValue("roomname", room.roomname);
    p.setValue("password", room.password);
    p.setValue("limit", room.limit);
    p.setValue("current", room.users.size());

    for(auto socket : inHalls) socket->write(p);
}

void ServerRoomHandler::notifyDeleteRoom(const ServerRoomHandler::Room& room)
{
    Packet p(Packet::room);
    p.setValue("type", "notify");
    p.setValue("operation", "delete");
    p.setValue("roomid", room.id);
    p.setValue("username", room.username);
    p.setValue("roomname", room.roomname);
    p.setValue("password", room.password);
    p.setValue("limit", room.limit);
    p.setValue("current", room.users.size());

    for(auto socket : inHalls) socket->write(p);
}

void ServerRoomHandler::notifyModifyRoom(const ServerRoomHandler::Room& room)
{
    Packet p(Packet::room);
    p.setValue("type", "notify");
    p.setValue("operation", "modify");
    p.setValue("roomid", room.id);
    p.setValue("username", room.username);
    p.setValue("roomname", room.roomname);
    p.setValue("password", room.password);
    p.setValue("limit", room.limit);
    p.setValue("current", room.users.size());

    for(auto socket : inHalls) socket->write(p);
}

void ServerRoomHandler::notifyUser(const ServerRoomHandler::Room& room, const QString& username, bool add)
{
    Packet p(Packet::room);
    p.setValue("type", "notifyUser");
    p.setValue("operation", (add ? "add" : "del"));
    p.setValue("roomid", room.id);
    p.setValue("username", username);

    for(auto socket : inHalls) socket->write(p);
}
