QT       += core gui multimedia network

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
    ../common/mediacodec.cpp \
    ../common/mediacontent.cpp \
    ../common/mediaiodevice.cpp \
    ../common/mediamuxer.cpp \
    ../common/mediaplayer.cpp \
    ../common/mediaprocessor.cpp \
    ../common/mediaqueue.cpp \
    ../common/mediarecorder.cpp \
    ../common/mediastream.cpp \
    ../common/mediatimer.cpp \
    ../common/mediautil.cpp \
    ../common/mediavideowidget.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    ../common/mediacodec.h \
    ../common/mediacontent.h \
    ../common/mediaiodevice.h \
    ../common/mediamuxer.h \
    ../common/mediaplayer.h \
    ../common/mediaprocessor.h \
    ../common/mediaqueue.h \
    ../common/mediarecorder.h \
    ../common/mediastream.h \
    ../common/mediatimer.h \
    ../common/mediautil.h \
    ../common/mediavideowidget.h \
    widget.h

FORMS += \
    widget.ui

INCLUDEPATH += ../ffmpeg/include
INCLUDEPATH += ../common
DEPENDPATH += ../ffmpeg/include

LIBS += -L../ffmpeg/lib/
LIBS += -lswscale -lswresample -lpostproc -lavutil
LIBS += -lavformat -lavfilter -lavdevice -lavcodec

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
