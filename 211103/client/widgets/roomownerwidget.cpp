#include "roomownerwidget.h"
#include "ui_roomownerwidget.h"
#include "clientsocket.h"

#include <QCloseEvent>
#include <QAudioDeviceInfo>

RoomOwnerWidget::RoomOwnerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RoomOwnerWidget), roomid(-1)
{
    ui->setupUi(this);

    barrageManager = new BarrageManager(ui->wVideoWidget, this);

    camera = new QCamera(this);
    camera->setViewfinder(ui->wVideoWidget);

    cprobe = new QVideoProbe(this);
    cprobe->setSource(camera);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    QAudioFormat format;
    format.setCodec("audio/pcm");
    format.setSampleRate(44100);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setChannelCount(2);

    input = new QAudioInput(info, format, this);
    device = nullptr;

    QObject::connect(ui->btnSend, SIGNAL(clicked()),
                     this, SLOT(onChat()));
    QObject::connect(ui->leInput, SIGNAL(returnPressed()),
                     this, SLOT(onChat()));

    QObject::connect(ui->cbVideo, SIGNAL(stateChanged(int)),
                     this, SLOT(onCameraClicked(int)));
    QObject::connect(ui->cbAudio, SIGNAL(stateChanged(int)),
                     this, SLOT(onAudioClicked(int)));

    QObject::connect(cprobe, SIGNAL(videoFrameProbed(QVideoFrame)),
                     this, SLOT(onVideoFrameProbed(QVideoFrame)));
}

RoomOwnerWidget::~RoomOwnerWidget()
{
    delete ui;
}

void RoomOwnerWidget::setUsername(const QString& username)
{
    this->username = username;

    ui->label->setText("你好、" + username);
}

void RoomOwnerWidget::addUserList(const QString& username)
{
    ui->lwUsers->addItem(username);
}

void RoomOwnerWidget::delUserList(const QString& username)
{
    for(int i = 0; i < ui->lwUsers->count(); ++i) {
        if(ui->lwUsers->item(i)->text() == username) {
            delete ui->lwUsers->takeItem(i);
            break;
        }
    }
}

void RoomOwnerWidget::appendMessage(const QString& message)
{
    ui->tbShow->append(message);
}

void RoomOwnerWidget::sendGiftMessage(const QString& from, const QString& name)
{
    if(roomid < 0) return;

    QString message = "感谢" + from + "送的" + name;
    SingleTon<ClientSocket>::getReference().roomHander()->message(roomid, message);
}

void RoomOwnerWidget::appendBarrage(const QString& barrage)
{
    barrageManager->append(barrage);
}

void RoomOwnerWidget::onChat()
{
    QString message = ui->leInput->text();
    if(message.isEmpty()) return;

    ui->leInput->clear();

    SingleTon<ClientSocket>::getReference().roomHander()->chat(roomid, username, message);
}

void RoomOwnerWidget::onCameraClicked(int state)
{
    if(state) {
        // 打开
        camera->start();
    }else {
        // 关闭
        camera->stop();
    }
}

void RoomOwnerWidget::onAudioClicked(int state)
{
    if(state) {
        // 打开
        device = input->start();
        QObject::connect(device, SIGNAL(readyRead()),
                         this, SLOT(onAudioReadyRead()));
    }else {
        input->stop();
        device = nullptr;
    }
}

void RoomOwnerWidget::onVideoFrameProbed(QVideoFrame frame)
{
    frame.map(QAbstractVideoBuffer::ReadOnly);
    QImage image(frame.bits(), frame.width(), frame.height(), frame.bytesPerLine(),
                 QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));

    image = image.mirrored(false, true);
    SingleTon<ClientSocket>::getReference().roomHander()->video(roomid, image);
}

void RoomOwnerWidget::onAudioReadyRead()
{
    if(device) {
        SingleTon<ClientSocket>::getReference().roomHander()->audio(roomid, device->readAll());
    }
}

void RoomOwnerWidget::closeEvent(QCloseEvent* event)
{
    SingleTon<ClientSocket>::getReference().roomHander()->quitRoom(roomid, username);
    ui->cbVideo->setCheckState(Qt::Unchecked);
    ui->cbAudio->setCheckState(Qt::Unchecked);
    ui->leInput->clear();
    ui->tbShow->clear();
    ui->lwUsers->clear();

    event->accept();
}
