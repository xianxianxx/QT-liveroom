#ifndef SQLDAO_H
#define SQLDAO_H

#include "singleton.h"

#include <QSqlDatabase>

class SqlDao
{
friend class SingleTon<SqlDao>;

private:
    SqlDao();
    SqlDao(const SqlDao&)=delete;
    SqlDao(SqlDao&&)=delete;

    SqlDao& operator=(const SqlDao&)=delete;
    SqlDao& operator=(SqlDao&&)=delete;

    QSqlDatabase db;

public:
    inline bool isOpen() const{ return db.isOpen(); }
    void open();
    inline void close(){ db.close(); }

    inline bool transaction() { return db.transaction(); }
    inline bool commit() { return db.commit(); }
    inline bool rollback() { return db.rollback(); }

    bool insertUser(const QString& username, const QString& password);
    QString selectUserPassword(const QString& username);
    int selectMoney(const QString& username);
    bool setMoney(const QString& username, int money);
    bool addMoney(const QString& username, int money);
};

#endif // SQLDAO_H
