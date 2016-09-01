QT       += core gui multimedia printsupport multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = analog_signature_analyzer
VERSION = 1.0.0

TEMPLATE = app
DESTDIR = ../build
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

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
    common_types.cpp \
    formoptions.cpp \
    formabout.cpp

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
    settings.h \
    formoptions.h \
    formabout.h

FORMS    += mainwindow.ui \
    formcalibration.ui \
    formraw.ui \
    formdiagnose.ui \
    formanalyze.ui \
    formoptions.ui \
    formabout.ui

RESOURCES += \
    analog_signature_analyzer.qrc

DISTFILES += \
    run_debug_version.bat \
    icons/license.txt \
    asa.desktop \
    languages/lang_ru_RU.ts \
    languages/lang_en_US.ts

win32 {
    LIBS += -L$$PWD/zlib/win32-compiled/lib/ -lzdll
    INCLUDEPATH += $$PWD/zlib/win32-compiled/include
    DEPENDPATH += $$PWD/zlib/win32-compiled/include
}
unix {
    # TODO try to use zlib from system's filesystem
    INCLUDEPATH += $$PWD/zlib
    include($$PWD/zlib/zlib.pri)
}

include($$PWD/tiff/libtiff.pri)

win32 {
    RC_ICONS += icons/app_icon.ico
    QMAKE_TARGET_DESCRIPTION = "Analog signature analyzer"
    QMAKE_TARGET_COPYRIGHT = "Opensource"
    QMAKE_TARGET_PRODUCT = "Analog signature analyzer"
}

# DEPLOY
unix {
    system(mkdir -p $${DESTDIR}/languages)
    system(lupdate -verbose . -ts languages/lang_ru_RU.ts)
    system(lupdate -verbose . -ts languages/lang_en_US.ts)
    system(lrelease languages/lang_ru_RU.ts languages/lang_ru_RU.qm)
    system(lrelease languages/lang_en_US.ts languages/lang_en_US.qm)
    system(cp languages/*.qm $${DESTDIR}/languages/)
    first.commands = @echo ************Deploy application************ $$escape_expand(\n\t) \
                     cp $${PWD}/icons/app_icon.png $${DESTDIR} $$escape_expand(\n\t) \
                     cp $${PWD}/asa.desktop $${DESTDIR}
    QMAKE_EXTRA_TARGETS += first
}
win32 {
    translationsSource = $$PWD/languages
    translationsSource = $$replace(translationsSource, /, \\)
    resourcesTarget = $$DESTDIR
    resourcesTarget = $$replace(resourcesTarget, /, \\)
    system(mkdir $$resourcesTarget)
    system(mkdir $$resourcesTarget\\languages)
    system(lupdate -verbose . -ts languages\\lang_ru_RU.ts)
    system(lupdate -verbose . -ts languages\\lang_en_US.ts)
    system(lrelease languages\\lang_ru_RU.ts languages\\lang_ru_RU.qm)
    system(lrelease languages\\lang_en_US.ts languages\\lang_en_US.qm)
    system(xcopy /Y /R /V $$translationsSource\\lang_ru_RU.qm $$resourcesTarget\\languages\\)
    system(xcopy /Y /R /V $$translationsSource\\lang_en_US.qm $$resourcesTarget\\languages\\)
    debug_bat = $$PWD/run_debug_version.bat
    debug_bat = $$replace(debug_bat, /, \\)
    zlib_dll = $$PWD/zlib/win32-compiled/zlib1.dll
    zlib_dll = $$replace(zlib_dll, /, \\)
    system(xcopy /V /R /Y $$debug_bat $$resourcesTarget)
    system(xcopy /V /R /Y $$zlib_dll $$resourcesTarget)

    # Qt libraries
#    first.commands = @echo ************Copy libraries************ $$escape_expand(\n\t) \
#                     windeployqt $${DESTDIR} $$escape_expand(\n\t)
#    QMAKE_EXTRA_TARGETS += first
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Core.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Gui.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5PrintSupport.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Multimedia.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5MultimediaWidgets.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Network.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Widgets.dll" $$resourcesTarget)
    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5OpenGL.dll" $$resourcesTarget)
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
