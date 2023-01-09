#ifndef ROOMOWNERWIDGET_H
#define ROOMOWNERWIDGET_H

#include <QWidget>
#include <QCamera>
#include <QVideoProbe>
#include <QAudioInput>
#include <QAudioProbe>

#include "barrage.h"

namespace Ui {
class RoomOwnerWidget;
}

class RoomOwnerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RoomOwnerWidget(QWidget *parent = nullptr);
    ~RoomOwnerWidget();

    void setUsername(const QString& username);
    inline void setRoomId(int id) { roomid = id; }

    inline QString getUsername() const{ return this->username; }
    inline int getRoomId() const{ return this->roomid; }

    void addUserList(const QString& username);
    void delUserList(const QString& username);
    void appendMessage(const QString& message);
    void sendGiftMessage(const QString& from, const QString& name);
    void appendBarrage(const QString& barrage);

private slots:
    void onChat();
    void onCameraClicked(int state);
    void onAudioClicked(int state);
    void onVideoFrameProbed(QVideoFrame frame);
    void onAudioReadyRead();

protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    Ui::RoomOwnerWidget *ui;

    BarrageManager* barrageManager;

    QCamera* camera;
    QVideoProbe* cprobe;

    QAudioInput* input;
    QIODevice* device;

    QString username;
    int roomid;
};

#endif // ROOMOWNERWIDGET_H
