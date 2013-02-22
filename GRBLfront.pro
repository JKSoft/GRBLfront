#-------------------------------------------------
#
# Project created by QtCreator 2013-02-12T22:49:53
#
#-------------------------------------------------

QT       += core gui \
	    opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GRBLfront
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    globals.cpp \
    Serial.cpp \
    glwidget.cpp

HEADERS  += mainwindow.h \
    globals.h \
    Serial.h \
    glwidget.h

LIBS	+= Advapi32.lib

FORMS    += mainwindow.ui
