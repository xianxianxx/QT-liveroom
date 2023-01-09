#include "hallwidget.h"
#include "ui_hallwidget.h"

#include "createroomdialog.h"
#include "clientsocket.h"
#include "passworddialog.h"
#include "chargedialog.h"
#include "widgetmanager.h"

#include <QDebug>
#include <QMessageBox>

Room::Room()
    : roomid(-1), current(-1), limit(-1)
{

}

Room::Room(int roomid, const QString& roomname, const QString& username, const QString& password, int current, int limit)
    : roomid(roomid), roomname(roomname), username(username),
      password(password), current(current), limit(limit)
{

}

QString Room::toString() const
{
    QString title; // 房间号: 0 房间名: xxx 房主: xxx 密码 : 是/否 人数[1/100];
    title += "房间号: " + QString::number(roomid);
    title += " 房间名: " + roomname;
    title += " 房主: " + username;
    title += QString(" 密码: ") + (password.isEmpty() ? "否" : "是");
    title += " 人数[" + QString::number(current)
              + '/' + QString::number(limit) + ']';

    return title;
}

HallWidget::HallWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HallWidget)
{
    ui->setupUi(this);

    QObject::connect(ui->btnCreate, SIGNAL(clicked()),
                     this, SLOT(onBtnCreateRoomClicked()));

    QObject::connect(ui->lwRooms, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                     this, SLOT(onItemDoubleClicked(QListWidgetItem*)));

    QObject::connect(ui->btnCharge, SIGNAL(clicked()),
                     this, SLOT(onBtnChargeClicked()));
}

void HallWidget::setUsername(const QString& username)
{
    this->username = username;
    this->setWindowTitle("你好: " + username);
}

void HallWidget::appendRoomList(const QList<Room>& rooms)
{
    ui->lwRooms->clear();

    for(auto& room : rooms) {
        QListWidgetItem* item = new QListWidgetItem(room.toString(), ui->lwRooms);
        item->setData(Qt::UserRole + 0, room.getRoomid());
        item->setData(Qt::UserRole + 1, room.getRoomname());
        item->setData(Qt::UserRole + 2, room.getUsername());
        item->setData(Qt::UserRole + 3, room.getPassword());
        item->setData(Qt::UserRole + 4, room.getCurrent());
        item->setData(Qt::UserRole + 5, room.getLimit());

        ui->lwRooms->addItem(item);
    }
}

void HallWidget::addNewRoom(const Room& room)
{
    QListWidgetItem* item = new QListWidgetItem(room.toString(), ui->lwRooms);
    item->setData(Qt::UserRole + 0, room.getRoomid());
    item->setData(Qt::UserRole + 1, room.getRoomname());
    item->setData(Qt::UserRole + 2, room.getUsername());
    item->setData(Qt::UserRole + 3, room.getPassword());
    item->setData(Qt::UserRole + 4, room.getCurrent());
    item->setData(Qt::UserRole + 5, room.getLimit());

    ui->lwRooms->addItem(item);
}

void HallWidget::deleteRoom(const Room& room)
{
    int roomid = room.getRoomid();

    for(int i = 0; i < ui->lwRooms->count(); ++i) {
        auto item = ui->lwRooms->item(i);
        int id = item->data(Qt::UserRole + 0).toInt();

        if(id == roomid) {
            // 我要删除的目标
            delete ui->lwRooms->takeItem(i);
            break;
        }
    }
}

void HallWidget::modifyRoom(const Room& room)
{
    int roomid = room.getRoomid();

    for(int i = 0; i < ui->lwRooms->count(); ++i) {
        auto item = ui->lwRooms->item(i);
        int id = item->data(Qt::UserRole + 0).toInt();

        if(id == roomid) {
            // 我要删除的目标
            item->setData(Qt::UserRole + 1, room.getRoomname());
            item->setData(Qt::UserRole + 2, room.getUsername());
            item->setData(Qt::UserRole + 3, room.getPassword());
            item->setData(Qt::UserRole + 4, room.getCurrent());
            item->setData(Qt::UserRole + 5, room.getLimit());

            item->setText(room.toString());
            break;
        }
    }
}

void HallWidget::setMoney(int money)
{
    ui->lbCharge->setNum(money);
}

void HallWidget::onBtnCreateRoomClicked()
{
    CreateRoomDialog dialog;
    if(dialog.exec() == 0) return;

    // 接受，从dialog中获取需要的信息，发送给服务器，作为创建房间
    // 房间名 房主名 人数 密码
    QString roomname = dialog.getRoomName();
    QString password = dialog.getPassword();
    int limit = dialog.getLimit();

    SingleTon<ClientSocket>::getReference().roomHander()->create(username, roomname, password, limit);
}

void HallWidget::onItemDoubleClicked(QListWidgetItem* item)
{
    int roomid = item->data(Qt::UserRole + 0).toInt();

    // 先判断是否已经在房间里
    if(SingleTon<WidgetManager>::getReference().roomOwnerWidget()->getRoomId() == roomid) {
        SingleTon<WidgetManager>::getReference().roomOwnerWidget()->activateWindow();
        SingleTon<WidgetManager>::getReference().roomOwnerWidget()->showNormal();
        return;
    }

    RoomGuestWidget* widget = SingleTon<WidgetManager>::getReference().selectRoomGuestWidget(roomid);
    if(widget != nullptr) {
        widget->activateWindow();
        widget->showNormal();
        return;
    }

    QString password = item->data(Qt::UserRole + 3).toString();
    if(!password.isEmpty()) {
        // 房间设置密码，需要用户输入密码，才允许进入房间
        PasswordDialog dialog;
        if(dialog.exec() == false) {
            // 取消
            return;
        }

        password = dialog.getPassword();
    }

    SingleTon<ClientSocket>::getReference().roomHander()->intoRoom(roomid, username, password);
}

void HallWidget::onBtnChargeClicked()
{
    ChargeDialog dialog;
    if(dialog.exec()) {
        // 确定
        int money = dialog.getMoney();
        SingleTon<ClientSocket>::getReference().userHander()->charge(username, money);
    }
}

HallWidget::~HallWidget()
{
    delete ui;
}


void HallWidget::showEvent(QShowEvent*)
{
    SingleTon<ClientSocket>::getReference().roomHander()->inHall(true);
    SingleTon<ClientSocket>::getReference().roomHander()->reloadRoom();
}

void HallWidget::hideEvent(QHideEvent*)
{
    SingleTon<ClientSocket>::getReference().roomHander()->inHall(false);
}
