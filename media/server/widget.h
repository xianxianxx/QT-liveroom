#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

#include "mediarecorder.h"
#include "mediaiodevice.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

signals:
    void sendStream(QByteArray stream);

private slots:
    void onMediaReadyRead();
    void onSendStream(QByteArray stream);

    void onNewConnection();
    void onDisConnected();

    void on_btnStart_clicked();

private:
    Ui::Widget *ui;

    QTcpServer* server;
    QList<QTcpSocket*> sockets;

    MediaRecorder* recorder;
    MediaIODevice* device;
};
#endif // WIDGET_H
