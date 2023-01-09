#ifndef ROOMGUESTWIDGET_H
#define ROOMGUESTWIDGET_H

#include <QWidget>
#include <QMenu>
#include <QAudioOutput>
#include <QAudioBuffer>

#include "barrage.h"

namespace Ui {
class RoomGuestWidget;
}

class RoomGuestWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RoomGuestWidget(QWidget *parent = nullptr);
    ~RoomGuestWidget();

    void setUsername(const QString& username);
    void setOwnername(const QString& ownername);
    inline void setRoomId(int id) { roomid = id; }

    inline QString getUsername() const{ return this->username; }
    inline QString getOwnername() const{ return this->ownername; }
    inline int getRoomId() const{ return this->roomid; }

    void reloadUsers(const QStringList& users);
    void addUserList(const QString& username);
    void delUserList(const QString& username);
    void appendMessage(const QString& message);
    void appendBarrage(const QString& barrage);
    void setImage(const QImage& image);
    void setAudio(const QByteArray& audio);

private slots:
    void onChat();
    void onGift(QAction* action);

protected:
    virtual void closeEvent(QCloseEvent*) override;
    virtual void showEvent(QShowEvent *event) override;

private:
    Ui::RoomGuestWidget *ui;
    BarrageManager* barrageManager;

    QMenu* giftMenu;

    QAudioOutput* output;
    QIODevice* device;

    QString username;
    QString ownername;

    int roomid;
};

#endif // ROOMGUESTWIDGET_H
