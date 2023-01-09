QT       += core gui network multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../common/network/handlecontext.cpp \
    ../common/network/handler.cpp \
    ../common/network/packer.cpp \
    ../common/network/packet.cpp \
    clientsocket.cpp \
    handlers/clientroomhandler.cpp \
    handlers/clientuserhandler.cpp \
    main.cpp \
    widgets/barrage.cpp \
    widgets/chargedialog.cpp \
    widgets/createroomdialog.cpp \
    widgets/hallwidget.cpp \
    widgets/loginwidget.cpp \
    widgets/passworddialog.cpp \
    widgets/roomguestwidget.cpp \
    widgets/roomownerwidget.cpp \
    widgets/videowidget.cpp \
    widgets/widgetmanager.cpp

HEADERS += \
    ../common/network/handlecontext.h \
    ../common/network/handler.h \
    ../common/network/packer.h \
    ../common/network/packet.h \
    ../common/tool/singleton.h \
    clientsocket.h \
    handlers/clientroomhandler.h \
    handlers/clientuserhandler.h \
    widgets/barrage.h \
    widgets/chargedialog.h \
    widgets/createroomdialog.h \
    widgets/hallwidget.h \
    widgets/loginwidget.h \
    widgets/passworddialog.h \
    widgets/roomguestwidget.h \
    widgets/roomownerwidget.h \
    widgets/videowidget.h \
    widgets/widgetmanager.h

FORMS += \
    widgets/chargedialog.ui \
    widgets/createroomdialog.ui \
    widgets/hallwidget.ui \
    widgets/loginwidget.ui \
    widgets/passworddialog.ui \
    widgets/roomguestwidget.ui \
    widgets/roomownerwidget.ui

INCLUDEPATH += ../common/network
INCLUDEPATH += ../common/tool
DEPENDPATH += ../common/network
DEPENDPATH += ../common/tool

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
