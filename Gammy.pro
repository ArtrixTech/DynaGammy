#-------------------------------------------------
#
# Project created by QtCreator 2019-04-07T19:13:04
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
message(Qt version: $$[QT_VERSION])

TARGET = gammy
TEMPLATE = app

CONFIG += c++11
CONFIG += c++17

SOURCES += src/main.cpp src/mainwindow.cpp
HEADERS += src/main.h src/mainwindow.h
FORMS   += src/mainwindow.ui

win32
{
    SOURCES += src/dxgidupl.cpp
    HEADERS += src/dxgidupl.h \
                   winres.h winres.rc
    RC_FILE = winres.rc
    RESOURCES += res.qrc
}

unix #Doesn't work
{
    HEADERS += src/x11.h
    SOURCES += src/x11.cpp
    LIBS += -lX11 -lXxf86vm
}

win32-msvc*
{
    HEADERS -= src/x11.h
    SOURCES -= src/x11.cpp
    LIBS -= -lX11 -lXxf86vm
}

RCC_DIR = $$PWD/res
UI_DIR = $$PWD/res
MOC_DIR = $$PWD/res/tmp
OBJECTS_DIR = $$PWD/res/tmp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Make the compiler emit warnings for deprecated Qt features
DEFINES += QT_DEPRECATED_WARNINGS

# Uncomment to make the code fail to compile if you use deprecated APIs.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
