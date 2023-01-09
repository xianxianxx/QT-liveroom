#include "widget.h"

#include <QApplication>

extern "C"
{
#include "libavdevice/avdevice.h"
}

int main(int argc, char *argv[])
{
    avdevice_register_all();
    qRegisterMetaType<int64_t>("int64_t");

    QApplication a(argc, argv);

    Widget w;
    w.show();

    return a.exec();
}
