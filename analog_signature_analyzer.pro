QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = analog_signature_analyzer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ToneGenerator.cpp

HEADERS  += mainwindow.h \
    ToneGenerator.h

FORMS    += mainwindow.ui
