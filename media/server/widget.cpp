#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    server = new QTcpServer(this);
    QObject::connect(server, &QTcpServer::newConnection, this, &Widget::onNewConnection);

    server->listen(QHostAddress("127.0.0.1"), 22222);

    QObject::connect(this, &Widget::sendStream, this, &Widget::onSendStream);

    recorder = new MediaRecorder(this);
    recorder->setMedia(QCameraInfo::defaultCamera(), QAudioDeviceInfo::availableDevices(QAudio::AudioInput).first());
    recorder->setVideoOutput(ui->widget);

    device = new MediaIODevice(this);

    QObject::connect(device, &MediaIODevice::readyRead, this, &Widget::onMediaReadyRead);
    QObject::connect(ui->btnPrev, &QPushButton::clicked, [=](){
        recorder->preview();
    });
}

Widget::~Widget()
{
    delete ui;
}

void Widget::onMediaReadyRead()
{
    auto stream = device->readAll();
    emit sendStream(std::move(stream));
}

void Widget::onSendStream(QByteArray stream)
{
    for(auto s : sockets) s->write(stream);
}

void Widget::onNewConnection()
{
    while(server->hasPendingConnections()) {
        QTcpSocket* s = server->nextPendingConnection();
        if(s){
            sockets.append(s);
        }
    }
}

void Widget::onDisConnected()
{
    QTcpSocket* s = static_cast<QTcpSocket*>(QObject::sender());
    s->deleteLater();
    sockets.removeOne(s);
}

void Widget::on_btnStart_clicked()
{
    recorder->record(device);
}
