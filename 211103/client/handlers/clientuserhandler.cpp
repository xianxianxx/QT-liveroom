#include "clientuserhandler.h"

#include <QMessageBox>

#include "widgets/widgetmanager.h"

ClientUserHandler::ClientUserHandler(QTcpSocket* socket, Packer* packer)
    : Handler(socket, packer)
{

}

void ClientUserHandler::handle(const Packet& packet)
{
    QString type = packet.getValue("type").toString();
    if(type == "login") {
        login(packet);
    }else if(type == "regist") {
        regist(packet);
    }else if(type == "force") {
        force(packet);
    }else if(type == "charge") {
        charge(packet);
    }else if(type == "gift") {
        gift(packet);
    }
}

void ClientUserHandler::regist(const QString& username, const QString& password)
{
    Packet packet(Packet::user);

    packet.setValue("type", "regist");
    packet.setValue("username", username);
    packet.setValue("password", password);

    socket()->write(packer()->pack(packet));
}

void ClientUserHandler::login(const QString& username, const QString& password)
{
    Packet packet(Packet::user);

    packet.setValue("type", "login");
    packet.setValue("username", username);
    packet.setValue("password", password);

    socket()->write(packer()->pack(packet));
}

void ClientUserHandler::login(const Packet& packet)
{
    int result = packet.getValue("result").toInt();
    QString info = packet.getValue("info").toString();
    if(result) {
        // 失败
        QMessageBox::warning(SingleTon<WidgetManager>::getReference().loginWidget(),
                             "登录信息", info);
    }else {
        // 成功
        // 界面切换
        QString username = packet.getValue("username").toString();
        int money = packet.getValue("money").toInt();

        SingleTon<WidgetManager>::getReference().hallWidget()->setUsername(username);
        SingleTon<WidgetManager>::getReference().hallWidget()->setMoney(money);
        SingleTon<WidgetManager>::getReference().hallWidget()->show();
        SingleTon<WidgetManager>::getReference().loginWidget()->hide();
    }
}

void ClientUserHandler::regist(const Packet& packet)
{
    int result = packet.getValue("result").toInt();
    QString info = packet.getValue("info").toString();
    if(result) {
        // 失败
        QMessageBox::warning(SingleTon<WidgetManager>::getInstance()->loginWidget(),
                             "注册信息", info);
    }else {
        // 成功
        QMessageBox::information(SingleTon<WidgetManager>::getInstance()->loginWidget(),
                             "注册信息", info);
    }
}

void ClientUserHandler::force(const Packet& packet)
{
    Q_UNUSED(packet);
    // 所有界面全部清空，只显示登录界面
    QMessageBox::information(SingleTon<WidgetManager>::getReference().loginWidget(),
                             "登录信息", "您已被踢出");

    SingleTon<WidgetManager>::getReference().clear();
    SingleTon<WidgetManager>::getReference().loginWidget()->show();
}

void ClientUserHandler::charge(const Packet& packet)
{
    int result = packet.getValue("result").toInt();
    if(result) {
        // 失败
        QMessageBox::warning(SingleTon<WidgetManager>::getReference().hallWidget(),
                             "充值信息", "充值失败");
    }else {
        int money = packet.getValue("money").toInt();
        SingleTon<WidgetManager>::getReference().hallWidget()->setMoney(money);
    }
}

void ClientUserHandler::gift(const Packet& packet)
{
    int result = packet.getValue("result").toInt();
    if(result) {
        // 失败
        QMessageBox::warning(SingleTon<WidgetManager>::getReference().hallWidget(),
                             "送礼信息", packet.getValue("info").toString());
        return;
    }

    QString from = packet.getValue("from").toString();
    QString to = packet.getValue("to").toString();
    QString name = packet.getValue("name").toString();
//    int money = packet.getValue("money").toInt();
    int userMoney = packet.getValue("usermoney").toInt();

    QString username = SingleTon<WidgetManager>::getReference().hallWidget()->getUsername();
    if(username == from) {
        // 送礼者
        SingleTon<WidgetManager>::getReference().hallWidget()->setMoney(userMoney);
    }else if(username == to) {
        // 接受者
        SingleTon<WidgetManager>::getReference().hallWidget()->setMoney(userMoney);

        // 感谢
        SingleTon<WidgetManager>::getReference().roomOwnerWidget()->sendGiftMessage(from, name);
    }
}

void ClientUserHandler::charge(const QString& username, int money)
{
    Packet packet(Packet::user);

    packet.setValue("type", "charge");
    packet.setValue("username", username);
    packet.setValue("money", money);

    socket()->write(packer()->pack(packet));
}

void ClientUserHandler::gift(const QString& from, const QString& to, const QString& name, int money)
{
    Packet packet(Packet::user);

    packet.setValue("type", "gift");
    packet.setValue("from", from);
    packet.setValue("to", to);
    packet.setValue("name", name);
    packet.setValue("money", money);

    socket()->write(packer()->pack(packet));
}
