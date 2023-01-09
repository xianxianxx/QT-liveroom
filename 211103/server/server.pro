QT -= gui
QT += network sql

CONFIG += c++11 console
CONFIG -= app_bundle

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
        dao/sqldao.cpp \
        handlers/serverroomhandler.cpp \
        handlers/serveruserhandler.cpp \
        main.cpp \
        server.cpp \
        serversocket.cpp

HEADERS += \
    ../common/network/handlecontext.h \
    ../common/network/handler.h \
    ../common/network/packer.h \
    ../common/network/packet.h \
    dao/sqldao.h \
    handlers/serverroomhandler.h \
    handlers/serveruserhandler.h \
    server.h \
    serversocket.h

INCLUDEPATH += ../common/network
INCLUDEPATH += ../common/tool
DEPENDPATH += ../common/network
DEPENDPATH += ../common/tool

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
