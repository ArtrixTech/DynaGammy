#-------------------------------------------------
#
# Project created by QtCreator 2019-04-07T19:13:04
#
#-------------------------------------------------

message(Qt $$[QT_VERSION])

QT      += widgets dbus
TARGET   = gammy
TEMPLATE = app
CONFIG  += c++1z

CONFIG(release) {
    message(Release build)
    CONFIG += optimize_full
}

win32 {
    HEADERS += src/dspctl-dxgi.h
    SOURCES += src/dspctl-dxgi.cpp

    VERSION                  = 0.9.6.4
    RC_ICONS                 = data/icons/gammy.ico
    QMAKE_TARGET_PRODUCT     = Gammy
    QMAKE_TARGET_DESCRIPTION = Gammy
    QMAKE_TARGET_COPYRIGHT   = Copyright (C) Francesco Fusco
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
    desktop.files = data/gammy.desktop
    icons.path    = $$DATAPATH/pixmaps
    icons.files   = data/icons/gammy.png

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

RESOURCES   += data/res.qrc
UI_DIR       = $$PWD/src
RCC_DIR      = build/rcc
MOC_DIR      = build/moc
OBJECTS_DIR  = build/obj
INCLUDEPATH += $$PWD/include

DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

