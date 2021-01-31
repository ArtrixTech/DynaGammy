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

win32 {
    HEADERS += winres.h
    HEADERS += src/dspctl-dxgi.h
    SOURCES += src/dspctl-dxgi.cpp
    RC_FILE = winres.rc
}

unix {
    HEADERS += src/dspctl-xlib.h
    SOURCES += src/dspctl-xlib.cpp
    LIBS += -lX11 -lXxf86vm -lXext

    isEmpty(PREFIX) {
        PREFIX = /usr
    }

    isEmpty(BINDIR) {
        BINDIR = bin
    }

    isEmpty(DATADIR) {
        DATADIR = $$PREFIX/share
    }

    INSTALLPATH = $$absolute_path($$BINDIR, $$PREFIX)
    DATAPATH    = $$absolute_path($$DATADIR)

    message(Bin path: $$INSTALLPATH)
    message(Data path: $$DATAPATH)

    target.path   = $$INSTALLPATH
    desktop.path  = $$DATAPATH/applications
    desktop.files = gammy.desktop
    icons.path    = $$DATAPATH/pixmaps
    icons.files   = icons/gammy.png

    INSTALLS += target desktop icons
}

HEADERS += src/mainwindow.h src/utils.h \
    src/component.h \
    src/gammactl.h \
    src/mediator.h \
    src/tempscheduler.h \
    src/cfg.h \
    src/RangeSlider.h \
    src/defs.h

SOURCES += src/main.cpp src/mainwindow.cpp src/utils.cpp \
    src/component.cpp \
    src/gammactl.cpp \
    src/mediator.cpp \
    src/tempscheduler.cpp \
    src/cfg.cpp \
    src/RangeSlider.cpp

FORMS += src/mainwindow.ui \
    src/tempscheduler.ui \

RESOURCES += res.qrc

UI_DIR      = $$PWD/src
RCC_DIR     = res
MOC_DIR     = res/tmp
OBJECTS_DIR = res/tmp

DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$PWD/includes
