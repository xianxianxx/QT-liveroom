#include "widget.h"
#include "ui_widget.h"

#include <QCameraInfo>
#include <QAudioDeviceInfo>
#include <QDebug>
#include <QTimer>

#include "mediaiodevice.h"
#include "mediaplayer.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    socket = new QTcpSocket(this);
    socket->connectToHost("127.0.0.1", 22222);
    if(socket->waitForConnected(1000) == false) {
        qDebug() << "连接超时";
        exit(-1);
    }

    QObject::connect(socket, &QTcpSocket::readyRead, this, &Widget::onReadyRead);

    device = new MediaIODevice(this);

    player = new MediaPlayer(this);

    player->setMedia(MediaContent(device));
    player->setVideoOutput(ui->widget);
    player->play();
}

Widget::~Widget()
{
    delete ui;
    player->stop();
}

void Widget::onReadyRead()
{
    auto buffer = socket->readAll();

    device->write(buffer);
}

