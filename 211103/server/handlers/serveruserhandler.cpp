#include "serveruserhandler.h"
#include "packet.h"
#include "dao/sqldao.h"

#include <QDebug>

ServerUserHandler::UserContainer ServerUserHandler::userContainer;

ServerUserHandler::ServerUserHandler(ServerSocket* socket, Packer* packer)
    : Handler(socket->getSocket(), packer), socket(socket)
{

}


void ServerUserHandler::handle(const Packet& packet)
{
    QString type = packet.getValue("type").toString();
    if(type == "login") {
        login(packet);
    }else if(type == "regist") {
        regist(packet);
    }else if(type == "charge") {
        charge(packet);
    }else if(type == "gift") {
        gift(packet);
    }
}

void ServerUserHandler::reset()
{
    if(username.isEmpty()) return;

    userContainer.users.remove(username);
    username.clear();
}

void ServerUserHandler::regist(const Packet& packet)
{
    QString username = packet.getValue("username").toString();
    QString password = packet.getValue("password").toString();

    Packet p(Packet::user);
    p.setValue("type", "regist");

    bool ret = SingleTon<SqlDao>::getReference().insertUser(username, password);
    if(ret) {
        // 成功
        p.setValue("result", 0);
        p.setValue("info", "注册成功");
    }else {
        // 失败
        p.setValue("result", 1);
        p.setValue("info", "注册失败，重复注册");
    }

    socket->write(p);
}

void ServerUserHandler::login(const Packet& packet)
{
    QString username = packet.getValue("username").toString();
    QString password = packet.getValue("password").toString();

    QString ret = SingleTon<SqlDao>::getReference().selectUserPassword(username);

    Packet p(Packet::user);
    p.setValue("type", "login");

    do{
        if(ret.isEmpty()) {
            // 没找到
            p.setValue("result", 1);
            p.setValue("info", "登录失败，无此用户");
            break;
        }else if(ret != password) {
            // 密码错误
            p.setValue("result", 1);
            p.setValue("info", "登录失败，密码错误");
            break;
        }

        // 先判断是否重复登录
        auto it = userContainer.users.find(username);
        if(it != userContainer.users.end()) {
            // 重复的，需要通过it里面的socket通知现在在线的用户退出
            p.setValue("type", "force");
            it.value()->write(p);

            // 将对应username的serversocket状态清除
            it.value()->getContext()->reset();

            p.setValue("type", "login");
        }

        // 登录成功
        userContainer.users[username] = socket;
        this->username = username;

        int money = SingleTon<SqlDao>::getReference().selectMoney(username);

        p.setValue("username", username);
        p.setValue("money", money);
        p.setValue("result", 0);
        p.setValue("info", "登录成功");
    }while(0);

    socket->write(p);
}

void ServerUserHandler::charge(const Packet& packet)
{
    QString username = packet.getValue("username").toString();
    int money = packet.getValue("money").toInt();

    Packet p(Packet::user);
    p.setValue("type", "charge");

    do{
        if(money <= 0) {
            p.setValue("result", 1);
            p.setValue("info", "金额不能为负");
            break;
        }

        if(SingleTon<SqlDao>::getReference().addMoney(username, money) == false) {
            p.setValue("result", 1);
            p.setValue("info", "充值失败");
            break;
        }

        money = SingleTon<SqlDao>::getReference().selectMoney(username);
        p.setValue("money", money);
        p.setValue("result", 0);
    }while(0);

    socket->write(p);
}

void ServerUserHandler::gift(const Packet& packet)
{
    QString from = packet.getValue("from").toString();
    QString to = packet.getValue("to").toString();
    QString name = packet.getValue("name").toString();
    int money = packet.getValue("money").toInt();

    Packet p(Packet::user);
    p.setValue("type", "gift");

    do{
        // 1. 判断钱是否足够
        int fromMoney = SingleTon<SqlDao>::getReference().selectMoney(from);
        if(fromMoney < money) {
            // 钱不够
            p.setValue("result", 1);
            p.setValue("info", "送礼失败，余额不足");
            break;
        }

        SingleTon<SqlDao>::getReference().transaction();
        bool ret = true;
        ret = ret && SingleTon<SqlDao>::getReference().addMoney(from, -money);
        ret = ret && SingleTon<SqlDao>::getReference().addMoney(to, money);

        if(ret) SingleTon<SqlDao>::getReference().commit();
        else SingleTon<SqlDao>::getReference().rollback();

        if(ret) {
            // 成功
            p.setValue("result", 0);
            p.setValue("from", from);
            p.setValue("to", to);
            p.setValue("name", name);
            p.setValue("money", money);

            auto it = userContainer.users.find(to);
            if(it != userContainer.users.end()) {
                // 通知收到钱的用户
                p.setValue("usermoney", SingleTon<SqlDao>::getReference().selectMoney(to));

                it.value()->write(p);
            }

            p.setValue("usermoney", SingleTon<SqlDao>::getReference().selectMoney(from));
        }else {
            p.setValue("result", 1);
            p.setValue("info", "送礼失败");
        }
    }while(0);

    socket->write(p);
}
