QT       += core gui multimedia printsupport multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = analog_signature_analyzer
TEMPLATE = app
DESTDIR = ../build

SOURCES += main.cpp\
        mainwindow.cpp \
    ToneGenerator.cpp \
    qcustomplot/qcustomplot.cpp \
    OscilloscopeView.cpp \
    audioinputdevice.cpp \
    formcalibration.cpp \
    formraw.cpp \
    formdiagnose.cpp \
    boardview.cpp \
    signatureview.cpp \
    formanalyze.cpp \
    squarewidgetholder.cpp \
    smoothfilter.cpp \
    volumeindicator.cpp \
    settings.cpp \
    common_types.cpp

HEADERS  += mainwindow.h \
    ToneGenerator.h \
    qcustomplot/qcustomplot.h \
    OscilloscopeView.h \
    audioinputdevice.h \
    formcalibration.h \
    formraw.h \
    formdiagnose.h \
    boardview.h \
    signatureview.h \
    common_types.h \
    formanalyze.h \
    squarewidgetholder.h \
    smoothfilter.h \
    volumeindicator.h \
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

# DEPLOY
#translationsSource = $$PWD/languages/lang_ru_RU.qm
#database = $$PWD/asa.accdb
debug_bat = $$PWD/run_debug_version.bat
resourcesTarget = $$DESTDIR

win32 {
#    system(lupdate -verbose . -ts languages\\lang_ru_RU.ts)
#    system(lrelease languages\\lang_ru_RU.ts languages\\lang_ru_RU.qm)
#    translationsSource = $$replace(translationsSource, /, \\)
#    database = $$replace(database, /, \\)
    debug_bat = $$replace(debug_bat, /, \\)
    resourcesTarget = $$replace(resourcesTarget, /, \\)
    system(mkdir $$resourcesTarget)
#    system(xcopy /Y /V $$translationsSource $$resourcesTarget\\languages\\)
#    system(xcopy /Y /V $$database $$resourcesTarget\\)
    system(xcopy /Y /V $$debug_bat $$resourcesTarget\\)

    # Qt libraries
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Core.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Gui.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5PrintSupport.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Sql.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Multimedia.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Network.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Widgets.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\libgcc_s_dw2-1.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\libwinpthread-1.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\libstdc++-6.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\icu*.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\plugins\audio\qtaudio_windows.dll" $$resourcesTarget\audio\\)
#    system(xcopy /V /R /Y "%QTDIR%\plugins\sqldrivers\qsqlodbc.dll" $$resourcesTarget\sqldrivers\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\platforms\qminimal.dll" $$resourcesTarget\platforms\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\platforms\qoffscreen.dll" $$resourcesTarget\platforms\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\platforms\qwindows.dll" $$resourcesTarget\platforms\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\imageformats\qico.dll" $$resourcesTarget\imageformats\\)
    system(xcopy /V /R /Y "%QTDIR%\plugins\printsupport\windowsprintersupport.dll" $$resourcesTarget\printsupport\\)
}
