QT       += core gui multimedia printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = analog_signature_analyzer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ToneGenerator.cpp \
    audiocapture.cpp \
    qcustomplot/qcustomplot.cpp \
    OscilloscopeView.cpp

HEADERS  += mainwindow.h \
    ToneGenerator.h \
    audiocapture.h \
    qcustomplot/qcustomplot.h \
    OscilloscopeView.h

FORMS    += mainwindow.ui
