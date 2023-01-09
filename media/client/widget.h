#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QFile>

class MediaPlayer;
class MediaIODevice;

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

private:
    Ui::Widget *ui;

    QTcpSocket* socket;

    MediaPlayer* player;
    MediaIODevice* device;
};
#endif // WIDGET_H
