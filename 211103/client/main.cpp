#include <QApplication>

#include "clientsocket.h"
#include "widgets/widgetmanager.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SingleTon<ClientSocket>::getReference().connectToHost("127.0.0.1", 22223);
    SingleTon<WidgetManager>::getReference().loginWidget()->show();
//    SingleTon<WidgetManager>::getReference().hallWidget()->show();

    return a.exec();
}
