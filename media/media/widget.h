#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLocalServer>
#include <QLocalServer>

#include "mediaplayer.h"
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

private slots:
    void onReadyRead();
    void onPacketReceived(AVPacket* packet);
    void onPacketReceivedPrivate(AVPacket* packet);

signals:
    void transferPacket(AVPacket* packet);

private:
    Ui::Widget *ui;

    MediaPlayer* player;
    MediaRecorder* recorder;

    QFile file;
    MediaIODevice* device;
};
#endif // WIDGET_H
