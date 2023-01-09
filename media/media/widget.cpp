#include "widget.h"
#include "ui_widget.h"

#include <QCameraInfo>

extern "C"
{
#include "libavformat/avformat.h"
}

#define PLAYER 0
#define RECORDER 1
//#define
#define MTEST PLAYER

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

#if MTEST == PLAYER
    player = new MediaPlayer(this);
    player->setVideoOutput(ui->wVideoOutput);
//    player->setMedia(QUrl::fromLocalFile("d:/1.mp4"));
    player->setMedia(QUrl::fromLocalFile("d:/Users/jin/Downloads/地球百子.The.100.S07E01.中英字幕.WEBrip.720P-人人影视.mp4"));
//    player->setMedia(QCameraInfo::defaultCamera());
//    player->setMedia(QAudioDeviceInfo::availableDevices(QAudio::AudioInput).first());

    QObject::connect(ui->btnPlay, &QPushButton::clicked, [=](){
        player->play();
    });

    QObject::connect(ui->btnStop, &QPushButton::clicked, player, &MediaPlayer::stop);

    player->play();

#elif MTEST == RECORDER
    file.setFileName("d:/1.mp4");

    device = new MediaIODevice(this);

    QObject::connect(device, &MediaIODevice::readyRead, this, &Widget::onReadyRead);

    recorder = new MediaRecorder(this);
    recorder->setVideoOutput(ui->wVideoOutput);
    recorder->setMedia(QCameraInfo::defaultCamera(), QAudioDeviceInfo::availableDevices(QAudio::AudioInput).first());

    QObject::connect(recorder, &MediaRecorder::sendPacket,
                     this, &Widget::onPacketReceivedPrivate);
    QObject::connect(this, &Widget::transferPacket,
                     this, &Widget::onPacketReceived, Qt::QueuedConnection);

    QObject::connect(ui->btnPlay, &QPushButton::clicked, recorder, &MediaRecorder::preview);
    QObject::connect(recorder, &MediaRecorder::dbPresent, ui->progressBar, &QProgressBar::setValue);
    QObject::connect(ui->btnRecord, &QPushButton::clicked, [=](){
        file.open(QIODevice::WriteOnly | QIODevice::Truncate);

        recorder->record(device);
    });
    QObject::connect(ui->btnStop, &QPushButton::clicked, [=](){
        recorder->stopRecord();
        file.close();
    });

    recorder->preview();
#endif
}

Widget::~Widget()
{
    delete ui;
}

void Widget::onReadyRead()
{
    QByteArray array = device->readAll();
    qDebug() << "widget : " << array.size();
    file.write(array);
}

void Widget::onPacketReceived(AVPacket* packet)
{
//    qDebug() << packet->pts << ' ' << packet->dts << ' ' << packet->stream_index;

//    if(packet->stream_index == 0) file.write((const char*)packet->buf, packet->size);
    av_packet_unref(packet);
    av_packet_free(&packet);
}

void Widget::onPacketReceivedPrivate(AVPacket* packet)
{
    emit transferPacket(av_packet_clone(packet));
}

