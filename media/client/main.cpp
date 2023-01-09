#include "widget.h"

#include <QApplication>
#include <QTcpSocket>
#include <QDebug>
#include <QDateTime>
#include "singleton.h"

int main(int argc, char *argv[])
{
    qsrand(QDateTime::currentDateTime().toMSecsSinceEpoch());
    QApplication a(argc, argv);

    Widget w;
    w.show();

    return a.exec();
}
