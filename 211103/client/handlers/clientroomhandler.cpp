#include "clientroomhandler.h"
#include "widgets/widgetmanager.h"

#include <QMessageBox>
#include <QBuffer>

ClientRoomHandler::ClientRoomHandler(QTcpSocket* socket, Packer* packer)
    : Handler(socket, packer)
{

}

void ClientRoomHandler::handle(const Packet& packet)
{
    QString type = packet.getValue("type").toString();

    if(type == "create") {
        create(packet);
    }else if(type == "reloadRoom") {
        reloadRoom(packet);
    }else if(type == "notify") {
        notify(packet);
    }else if(type == "quitRoom") {
        quitRoom(packet);
    }else if(type == "intoRoom") {
        intoRoom(packet);
    }else if(type == "reloadUser") {
        reloadUser(packet);
    }else if(type == "notifyUser") {
        notifyUser(packet);
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

void ClientRoomHandler::create(const QString& username, const QString& roomname, const QString& password, int limit)
{
    Packet packet(Packet::room);

    packet.setValue("type", "create");
    packet.setValue("username", username);
    packet.setValue("roomname", roomname);
    packet.setValue("password", password);
    packet.setValue("limit", limit);

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::reloadRoom()
{
    Packet packet(Packet::room);

    packet.setValue("type", "reloadRoom");

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::inHall(bool in)
{
    Packet packet(Packet::room);

    packet.setValue("type", "inHall");
    packet.setValue("in", in);

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::quitRoom(int roomid, const QString& username)
{
    Packet packet(Packet::room);

    packet.setValue("type", "quitRoom");
    packet.setValue("roomid", roomid);
    packet.setValue("username", username);

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::intoRoom(int roomid, const QString& username, const QString& password)
{
    Packet packet(Packet::room);

    packet.setValue("type", "intoRoom");
    packet.setValue("roomid", roomid);
    packet.setValue("username", username);
    packet.setValue("password", password);

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::reloadUser(int roomid)
{
    Packet packet(Packet::room);

    packet.setValue("type", "reloadUser");
    packet.setValue("roomid", roomid);

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::chat(int roomid, const QString& username, const QString& message)
{
    Packet packet(Packet::room);

    packet.setValue("type", "chat");
    packet.setValue("roomid", roomid);
    packet.setValue("username", username);
    packet.setValue("message", message);

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::message(int roomid, const QString& message)
{
    Packet packet(Packet::room);

    packet.setValue("type", "message");
    packet.setValue("roomid", roomid);
    packet.setValue("message", message);

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::video(int roomid, const QImage& image)
{
    Packet packet(Packet::room);

    packet.setValue("type", "video");
    packet.setValue("roomid", roomid);

    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);

    image.save(&buffer, "JPG");

    packet.setValue("image", QString::fromUtf8(buffer.data().toBase64()));

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::audio(int roomid, const QByteArray& audio)
{
    Packet packet(Packet::room);

    packet.setValue("type", "audio");
    packet.setValue("roomid", roomid);

    packet.setValue("image", QString::fromUtf8(audio.toBase64()));

    socket()->write(packer()->pack(packet));
}

void ClientRoomHandler::create(const Packet& packet)
{
    int result = packet.getValue("result").toInt();
    if(result) {
        QMessageBox::warning(SingleTon<WidgetManager>::getReference().hallWidget(),
                             "创建房间信息", packet.getValue("info").toString());
        return;
    }

    QString username = packet.getValue("username").toString();
    int roomid = packet.getValue("roomid").toInt();
    QString roomname = packet.getValue("roomname").toString();
    QString password = packet.getValue("password").toString();
    int limit = packet.getValue("limit").toInt();

    QString title; // 房间号: 0 房间名: xxx 房主: xxx 密码 : 是/否 人数[1/100];
    title += "房间号: " + QString::number(roomid);
    title += " 房间名: " + roomname;
    title += " 房主: " + username;
    title += QString(" 密码: ") + (password.isEmpty() ? "否" : "是");
    title += " 人数[1/" + QString::number(limit) + ']';

    // 切换房间
    SingleTon<WidgetManager>::getReference().roomOwnerWidget()->setRoomId(roomid);
    SingleTon<WidgetManager>::getReference().roomOwnerWidget()->setUsername(username);
    SingleTon<WidgetManager>::getReference().roomOwnerWidget()->setWindowTitle(title);
    SingleTon<WidgetManager>::getReference().roomOwnerWidget()->addUserList(username);
    SingleTon<WidgetManager>::getReference().roomOwnerWidget()->show();
}

void ClientRoomHandler::reloadRoom(const Packet& packet)
{
    QVariantList rooms = packet.getValue("rooms").toList();

    QList<Room> list;
    for(auto& r : rooms) {
        const QVariantMap& v = r.toMap();

        list.append(Room(v["roomid"].toInt(), v["roomname"].toString(),
                v["username"].toString(), v["password"].toString(),
                v["current"].toInt(), v["limit"].toInt()));
    }

    SingleTon<WidgetManager>::getReference().hallWidget()->appendRoomList(list);
}

void ClientRoomHandler::notify(const Packet& packet)
{
    QString operation = packet.getValue("operation").toString();

    Room room(packet.getValue("roomid").toInt(), packet.getValue("roomname").toString(),
              packet.getValue("username").toString(), packet.getValue("password").toString(),
              packet.getValue("current").toInt(), packet.getValue("limit").toInt());

    if(operation == "new") {
        SingleTon<WidgetManager>::getReference().hallWidget()->addNewRoom(room);
    }else if(operation == "delete") {
        SingleTon<WidgetManager>::getReference().hallWidget()->deleteRoom(room);
    }else if(operation == "modify") {
        SingleTon<WidgetManager>::getReference().hallWidget()->modifyRoom(room);
    }
}

void ClientRoomHandler::quitRoom(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();
    QString operation = packet.getValue("operation").toString();

    // 1. 先找对应房间
    RoomGuestWidget* widget = SingleTon<WidgetManager>::getReference().selectRoomGuestWidget(roomid);
    if(widget != nullptr) {
        // 找到了
        if(operation == "force") {
            // 房主退出，才要被动退出
            QMessageBox::information(widget, "退出信息", "房主已退出，房间解散");
        }

        SingleTon<WidgetManager>::getReference().removeRoomGuestWidget(roomid);
        return;
    }

    if(SingleTon<WidgetManager>::getReference().roomOwnerWidget()->getRoomId() == roomid) {
        // 房主退出
        SingleTon<WidgetManager>::getReference().roomOwnerWidget()->hide();
    }
}

void ClientRoomHandler::intoRoom(const Packet& packet)
{
    int result = packet.getValue("result").toInt();
    if(result) {
        // 失败
        QMessageBox::warning(SingleTon<WidgetManager>::getReference().hallWidget(),
                             "进入房间信息", packet.getValue("info").toString());
    }else {
        // 成功
        int roomid = packet.getValue("roomid").toInt();
        QString username = packet.getValue("username").toString();
        QString ownername = packet.getValue("ownername").toString();

        RoomGuestWidget* widget = SingleTon<WidgetManager>::getReference().createRoomGuestWidget(roomid, username);
        if(widget != nullptr){
            widget->setOwnername(ownername);
            widget->show();
        }
    }
}

void ClientRoomHandler::reloadUser(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();
    QVariantList list = packet.getValue("users").toList();

    QStringList users;
    for(auto& l : list) {
        users.append(l.toString());
    }

    RoomGuestWidget* widget = SingleTon<WidgetManager>::getReference().selectRoomGuestWidget(roomid);
    if(widget != nullptr) {
        widget->reloadUsers(users);
    }
}

void ClientRoomHandler::notifyUser(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();
    QString operation = packet.getValue("operation").toString();
    QString username = packet.getValue("username").toString();

    // 1. 找到房间
    RoomGuestWidget* widget = SingleTon<WidgetManager>::getReference().selectRoomGuestWidget(roomid);
    if(widget != nullptr){
        if(operation == "add") {
            widget->addUserList(username);
        }else if(operation == "del") {
            widget->delUserList(username);
        }

        return;
    }

    if(SingleTon<WidgetManager>::getReference().roomOwnerWidget()->getRoomId() == roomid) {
        if(operation == "add") {
            SingleTon<WidgetManager>::getReference().roomOwnerWidget()->addUserList(username);
        }else if(operation == "del") {
            SingleTon<WidgetManager>::getReference().roomOwnerWidget()->delUserList(username);
        }
    }
}

void ClientRoomHandler::chat(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();

    QString username = packet.getValue("username").toString();
    QString datetime = packet.getValue("datetime").toString();
    QString message = packet.getValue("message").toString();

    // xxx (时间) :
    // 内容
    QString text = username + " (";
    text += datetime + "):\n";
    text += packet.getValue("message").toString();

    RoomGuestWidget* widget = SingleTon<WidgetManager>::getReference().selectRoomGuestWidget(roomid);
    if(widget != nullptr) {
        widget->appendMessage(text);
        widget->appendBarrage(message);
        return;
    }

    if(SingleTon<WidgetManager>::getReference().roomOwnerWidget()->getRoomId() == roomid) {
        SingleTon<WidgetManager>::getReference().roomOwnerWidget()->appendMessage(text);
        SingleTon<WidgetManager>::getReference().roomOwnerWidget()->appendBarrage(message);
    }
}

void ClientRoomHandler::message(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();

    QString message = packet.getValue("message").toString();

    RoomGuestWidget* widget = SingleTon<WidgetManager>::getReference().selectRoomGuestWidget(roomid);
    if(widget != nullptr) {
        widget->appendMessage(message);
        return;
    }

    if(SingleTon<WidgetManager>::getReference().roomOwnerWidget()->getRoomId() == roomid) {
        SingleTon<WidgetManager>::getReference().roomOwnerWidget()->appendMessage(message);
    }
}

void ClientRoomHandler::video(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();

    RoomGuestWidget* widget = SingleTon<WidgetManager>::getReference().selectRoomGuestWidget(roomid);
    if(widget == nullptr) return;

    QByteArray array = QByteArray::fromBase64(packet.getValue("image").toString().toUtf8());
    QImage image = QImage::fromData(array);

    widget->setImage(image);
}

void ClientRoomHandler::audio(const Packet& packet)
{
    int roomid = packet.getValue("roomid").toInt();

    RoomGuestWidget* widget = SingleTon<WidgetManager>::getReference().selectRoomGuestWidget(roomid);
    if(widget == nullptr) return;

    QVariantMap f = packet.getValue("format").toMap();

    QByteArray audio = QByteArray::fromBase64(packet.getValue("image").toString().toUtf8());

    widget->setAudio(audio);
}
