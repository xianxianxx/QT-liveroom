#include "sqldao.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

SqlDao::SqlDao()
{
    db = QSqlDatabase::addDatabase("QMYSQL");

    db.setUserName("root");
    db.setPassword("123");
    db.setDatabaseName("user");
}

void SqlDao::open()
{
    if(isOpen()) return;

    if(!db.open()) {
        qDebug() << "[SqlDao::open] 打开数据库失败 " << db.lastError().text();
    }
}

bool SqlDao::insertUser(const QString& username, const QString& password)
{
    open();

    QSqlQuery query;
    query.prepare("insert into user(username, password)values(:1, :2);");
    query.bindValue(":1", username);
    query.bindValue(":2", password);

    bool ret = query.exec();
    if(ret == false) {
        qDebug() << "[SqlDao::insertUser] 插入用户失败 " << query.lastError().text();
    }

    return ret;
}

QString SqlDao::selectUserPassword(const QString& username)
{
    open();

    QSqlQuery query;
    query.prepare("select password from user where username = :1;");
    query.bindValue(":1", username);

    QString password;
    bool ret = query.exec();
    if(ret == false) {
        qDebug() << "[SqlDao::insertUser] 插入用户失败 " << query.lastError().text();
    }else if(query.next()) {
        password = query.value(0).toString();
    }

    return password;
}

int SqlDao::selectMoney(const QString& username)
{
    open();
    int money = -1;

    QSqlQuery query;
    query.prepare("select money from user where username = :1;");
    query.bindValue(":1", username);

    bool ret = query.exec();
    if(ret == false) {
        qDebug() << "[SqlDao::insertUser] 插入用户失败 " << query.lastError().text();
    }else if(query.next()) {
        money = query.value(0).toInt();
    }

    return money;
}

bool SqlDao::setMoney(const QString& username, int money)
{
    open();

    QSqlQuery query;
    query.prepare("update user set money = :2 where username = :1;");
    query.bindValue(":1", username);
    query.bindValue(":2", money);

    bool ret = query.exec();
    if(ret == false) {
        qDebug() << "[SqlDao::insertUser] 插入用户失败 " << query.lastError().text();
    }

    return ret;
}

bool SqlDao::addMoney(const QString& username, int money)
{
    open();

    QSqlQuery query;
    query.prepare("update user set money = money + :2 where username = :1;");
    query.bindValue(":1", username);
    query.bindValue(":2", money);

    bool ret = query.exec();
    if(ret == false) {
        qDebug() << "[SqlDao::insertUser] 插入用户失败 " << query.lastError().text();
    }

    return ret;
}
