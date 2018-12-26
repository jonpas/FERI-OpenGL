QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OpenGL
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11
QMAKE_CXXFLAGS += -Wextra -pedantic

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    widgetopengldraw.cpp

HEADERS += \
    mainwindow.h \
    widgetopengldraw.h

FORMS += \
    mainwindow.ui
