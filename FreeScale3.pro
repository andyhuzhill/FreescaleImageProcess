#-------------------------------------------------
#
# Project created by QtCreator 2013-06-25T22:29:20
#
#-------------------------------------------------

QT       += core gui

TARGET = FreeScale3
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    pid.c

HEADERS  += mainwindow.h \
    pid.h

FORMS    += mainwindow.ui

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += opencv
