#include "widget.h"

extern "C"
{
#include "libavdevice/avdevice.h"
}

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    avdevice_register_all();

    QApplication a(argc, argv);

    Widget w;
    w.show();

    return a.exec();
}
