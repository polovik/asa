QT       += core gui multimedia printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = analog_signature_analyzer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ToneGenerator.cpp \
    qcustomplot/qcustomplot.cpp \
    OscilloscopeView.cpp \
    audioinputdevice.cpp

HEADERS  += mainwindow.h \
    ToneGenerator.h \
    qcustomplot/qcustomplot.h \
    OscilloscopeView.h \
    audioinputdevice.h

FORMS    += mainwindow.ui
