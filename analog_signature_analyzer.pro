QT       += core gui multimedia printsupport multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = analog_signature_analyzer
VERSION = 1.0.0

TEMPLATE = app
DESTDIR = ../build

SOURCES += main.cpp\
        mainwindow.cpp \
    devices/tonegenerator.cpp \
    qcustomplot/qcustomplot.cpp \
    widgets/oscilloscopeview.cpp \
    devices/audioinputdevice.cpp \
    formcalibration.cpp \
    formraw.cpp \
    formdiagnose.cpp \
    widgets/boardview.cpp \
    widgets/signatureview.cpp \
    formanalyze.cpp \
    widgets/squarewidgetholder.cpp \
    smoothfilter.cpp \
    widgets/volumeindicator.cpp \
    settings.cpp \
    common_types.cpp

HEADERS  += mainwindow.h \
    devices/tonegenerator.h \
    qcustomplot/qcustomplot.h \
    widgets/oscilloscopeview.h \
    devices/audioinputdevice.h \
    formcalibration.h \
    formraw.h \
    formdiagnose.h \
    widgets/boardview.h \
    widgets/signatureview.h \
    common_types.h \
    formanalyze.h \
    widgets/squarewidgetholder.h \
    smoothfilter.h \
    widgets/volumeindicator.h \
    settings.h

FORMS    += mainwindow.ui \
    formcalibration.ui \
    formraw.ui \
    formdiagnose.ui \
    formanalyze.ui

RESOURCES += \
    analog_signature_analyzer.qrc

DISTFILES += \
    run_debug_version.bat \
    icons/license.txt

include($$PWD/tiff/libtiff.pri)

win32 {
    RC_ICONS += icons/app_icon.ico
    QMAKE_TARGET_DESCRIPTION = "Analog signature analyzer"
    QMAKE_TARGET_COPYRIGHT = "Opensource"
    QMAKE_TARGET_PRODUCT = "Analog signature analyzer"
}

# DEPLOY
#translationsSource = $$PWD/languages/lang_ru_RU.qm
debug_bat = $$PWD/run_debug_version.bat
resourcesTarget = $$DESTDIR

win32 {
#    system(lupdate -verbose . -ts languages\\lang_ru_RU.ts)
#    system(lrelease languages\\lang_ru_RU.ts languages\\lang_ru_RU.qm)
#    translationsSource = $$replace(translationsSource, /, \\)
    debug_bat = $$replace(debug_bat, /, \\)
    resourcesTarget = $$replace(resourcesTarget, /, \\)
    system(mkdir $$resourcesTarget)
#    system(xcopy /Y /V $$translationsSource $$resourcesTarget\\languages\\)
    system(xcopy /Y /V $$debug_bat $$resourcesTarget\\)

    # Qt libraries
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Core.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Gui.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5PrintSupport.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Multimedia.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Network.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Widgets.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\libgcc_s_dw2-1.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\libwinpthread-1.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\libstdc++-6.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\icu*.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\plugins\audio\qtaudio_windows.dll" $$resourcesTarget\audio\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\platforms\qminimal.dll" $$resourcesTarget\platforms\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\platforms\qoffscreen.dll" $$resourcesTarget\platforms\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\platforms\qwindows.dll" $$resourcesTarget\platforms\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\imageformats\qico.dll" $$resourcesTarget\imageformats\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\printsupport\windowsprintersupport.dll" $$resourcesTarget\printsupport\\)
}
