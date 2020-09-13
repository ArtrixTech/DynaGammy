#-------------------------------------------------
#
# Project created by QtCreator 2019-04-07T19:13:04
#
#-------------------------------------------------

#QT       += core gui
QT += dbus
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
message(Qt version: $$[QT_VERSION])

TARGET = gammy
TEMPLATE = app

CONFIG += c++11 c++1z

CONFIG(release) {
    message(Release build)
    CONFIG += optimize_full
}

HEADERS += src/mainwindow.h src/utils.h \
    src/tempscheduler.h \
    src/cfg.h \
    src/RangeSlider.h \
    src/defs.h

SOURCES += src/main.cpp src/mainwindow.cpp src/utils.cpp \
    src/tempscheduler.cpp \
    src/cfg.cpp \
    src/RangeSlider.cpp

FORMS   += src/mainwindow.ui \
    src/tempscheduler.ui \

win32:{
    SOURCES += src/dxgidupl.cpp
    HEADERS += src/dxgidupl.h \
                   winres.h winres.rc
    RC_FILE = winres.rc
}

unix:{
    HEADERS += src/x11.h
    SOURCES += src/x11.cpp
    LIBS += -lX11 -lXxf86vm

    isEmpty(PREFIX) {
        PREFIX = /usr/opt
    }

    isEmpty(BINDIR) {
        BINDIR = bin
    }

    BINDIR = $$absolute_path($$BINDIR, $$PREFIX)
    target.path = $$BINDIR

    message(Install path: $$target.path)
}

RESOURCES += res.qrc

UI_DIR = $$PWD/src
RCC_DIR = res
MOC_DIR = res/tmp
OBJECTS_DIR = res/tmp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Make the compiler emit warnings for deprecated Qt features
DEFINES += QT_DEPRECATED_WARNINGS

# Uncomment to make the code fail to compile if you use deprecated APIs.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/includes
