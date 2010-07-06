TEMPLATE = app
TARGET = 
LIBS += -Llibvnc/libvncclient/.libs -lvncclient
DEFINES += QTONLY
CONFIG += silent

OBJECTS_DIR = $${PWD}/tmp
MOC_DIR = $${PWD}/tmp
VPATH = $${PWD}/src
INCLUDEPATH = $${PWD}/src

HEADERS += remoteview.h vncclientthread.h vncview.h mainwindow.h
SOURCES += main.cpp remoteview.cpp vncclientthread.cpp vncview.cpp
