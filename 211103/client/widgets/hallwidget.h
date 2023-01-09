#ifndef HALLWIDGET_H
#define HALLWIDGET_H

#include <QWidget>
#include <QListWidgetItem>

namespace Ui {
class HallWidget;
}

class Room
{
public:
    Room();
    Room(int roomid, const QString& roomname, const QString& username,
         const QString& password, int current, int limit);

    inline int getRoomid() const{ return this->roomid; }
    inline QString getRoomname() const{ return this->roomname; }
    inline QString getUsername() const{ return this->username; }
    inline QString getPassword() const{ return this->password; }
    inline int getCurrent() const{ return this->current; }
    inline int getLimit() const{ return this->limit; }

    inline void setRoomid(int roomid) { this->roomid = roomid; }
    inline void setRoomname(const QString& roomname){ this->roomname = roomname; }
    inline void setUsername(const QString& username){ this->username = username; }
    inline void setPassword(const QString& password){ this->password = password; }
    inline void setCurrent(int current){ this->current = current; }
    inline void setLimit(int limit){ this->limit = limit; }

    QString toString() const;

private:
    int roomid;
    QString roomname;
    QString username;
    QString password;
    int current;
    int limit;
};

class HallWidget : public QWidget
{
friend class WidgetManager;

    Q_OBJECT
private:
    explicit HallWidget(QWidget *parent = nullptr);
    HallWidget(const HallWidget&)=delete;
    HallWidget(HallWidget&&)=delete;
    HallWidget& operator=(const HallWidget&)=delete;
    HallWidget& operator=(HallWidget&&)=delete;

public:
    ~HallWidget();

    void setUsername(const QString& username);
    inline QString getUsername() const{ return this->username; }
    void appendRoomList(const QList<Room>& rooms);
    void addNewRoom(const Room& room);
    void deleteRoom(const Room& room);
    void modifyRoom(const Room& room);
    void setMoney(int money);

private slots:
    void onBtnCreateRoomClicked();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onBtnChargeClicked();

protected:
    virtual void showEvent(QShowEvent*) override;
    virtual void hideEvent(QHideEvent*) override;

private:
    Ui::HallWidget *ui;

    QString username;
};

#endif // HALLWIDGET_H
