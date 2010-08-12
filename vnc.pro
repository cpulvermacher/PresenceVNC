TEMPLATE = app
TARGET = presencevnc-bin
LIBS += -Llibvnc/libvncclient/.libs -lvncclient
DEFINES += QTONLY
QT += maemo5
CONFIG += silent release

OBJECTS_DIR = $${PWD}/tmp
MOC_DIR = $${PWD}/tmp
VPATH = $${PWD}/src
INCLUDEPATH = $${PWD}/src

HEADERS += remoteview.h vncclientthread.h vncview.h mainwindow.h preferences.h connectdialog.h
SOURCES += main.cpp remoteview.cpp vncclientthread.cpp vncview.cpp mainwindow.cpp preferences.cpp connectdialog.cpp
