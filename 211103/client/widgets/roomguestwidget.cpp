#include "roomguestwidget.h"
#include "ui_roomguestwidget.h"

#include "clientsocket.h"

#include <QAudioOutput>
#include <QAudioDeviceInfo>

RoomGuestWidget::RoomGuestWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RoomGuestWidget)
{
    ui->setupUi(this);

    barrageManager = new BarrageManager(ui->wVideoWidget, this);

    giftMenu = new QMenu(this);
    QFont font = giftMenu->font();
    font.setPixelSize(20);
    giftMenu->setFont(font);

    giftMenu->addAction("鱼丸");
    giftMenu->addAction("火箭");

    ui->btnGift->setMenu(giftMenu);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
    QAudioFormat format;
    format.setCodec("audio/pcm");
    format.setSampleRate(44100);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setChannelCount(2);

    output = new QAudioOutput(info, format, this);
    device = output->start();

    QObject::connect(ui->btnSend, SIGNAL(clicked()),
                     this, SLOT(onChat()));
    QObject::connect(ui->leInput, SIGNAL(returnPressed()),
                     this, SLOT(onChat()));
    QObject::connect(giftMenu, SIGNAL(triggered(QAction*)),
                     this, SLOT(onGift(QAction*)));
}

RoomGuestWidget::~RoomGuestWidget()
{
    delete ui;
}

void RoomGuestWidget::setUsername(const QString& username)
{
    this->username = username;
    ui->lbUser->setText("你好、" + username);
}

void RoomGuestWidget::setOwnername(const QString& ownername)
{
    this->ownername = ownername;
    ui->lbOwner->setText("房主：" + ownername);
}

void RoomGuestWidget::reloadUsers(const QStringList& users)
{
    ui->lwUsers->clear();
    ui->lwUsers->addItems(users);
}

void RoomGuestWidget::addUserList(const QString& username)
{
    ui->lwUsers->addItem(username);
}

void RoomGuestWidget::delUserList(const QString& username)
{
    for(int i = 0; i < ui->lwUsers->count(); ++i) {
        if(ui->lwUsers->item(i)->text() == username) {
            delete ui->lwUsers->takeItem(i);
            break;
        }
    }
}

void RoomGuestWidget::appendMessage(const QString& message)
{
    ui->tbShow->append(message);
}

void RoomGuestWidget::appendBarrage(const QString& barrage)
{
    barrageManager->append(barrage);
}

void RoomGuestWidget::setImage(const QImage& image)
{
    ui->wVideoWidget->setImage(image);
}

void RoomGuestWidget::setAudio(const QByteArray& audio)
{
    if(device) device->write(audio);
}

void RoomGuestWidget::onChat()
{
    QString message = ui->leInput->text();
    if(message.isEmpty()) return;

    ui->leInput->clear();

    SingleTon<ClientSocket>::getReference().roomHander()->chat(roomid, username, message);
}

void RoomGuestWidget::onGift(QAction* action)
{
    QString text = action->text();
    int money = 0;
    if(text == "鱼丸") money = 10;
    else if(text == "火箭")  money = 1000;

    SingleTon<ClientSocket>::getReference().userHander()->gift(username, ownername, text, money);
}

void RoomGuestWidget::closeEvent(QCloseEvent*)
{
    SingleTon<ClientSocket>::getReference().roomHander()->quitRoom(roomid, username);
}

void RoomGuestWidget::showEvent(QShowEvent*)
{
    SingleTon<ClientSocket>::getReference().roomHander()->reloadUser(roomid);
}
