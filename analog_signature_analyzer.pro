QT       += core gui multimedia printsupport multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# /usr/bin/asa is already provided by ksh package (on opensuse it is installed by default)
# TODO rename application's name and update packages rules
TARGET = asa
VERSION = 1.0.0

TEMPLATE = app
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
    widgets/smoothfilter.cpp \
    widgets/volumeindicator.cpp \
    settings.cpp \
    common_types.cpp \
    formoptions.cpp \
    formabout.cpp \
    widgets/FancyTabBar/fancytab.cpp \
    widgets/FancyTabBar/fancytabbar.cpp \
    widgets/FancyTabBar/stylehelper.cpp \
    tiff/imagetiff.cpp

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
    widgets/smoothfilter.h \
    widgets/volumeindicator.h \
    settings.h \
    formoptions.h \
    formabout.h \
    widgets/FancyTabBar/fancytab.h \
    widgets/FancyTabBar/fancytabbar.h \
    widgets/FancyTabBar/stylehelper.h \
    tiff/imagetiff.h

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
    languages/lang_en_US.ts \
    prepare_deb_package.sh

win32 {
    # zlib
    LIBS += -L$$PWD/../3rd-party/zlib/lib/ -lzdll
    INCLUDEPATH += $$PWD/../3rd-party/zlib/include
    DEPENDPATH += $$PWD/../3rd-party/zlib/include
    # libtiff
    LIBS += -L$$PWD/../3rd-party/libtiff-build/ -ltiff4
    INCLUDEPATH += $$PWD/../3rd-party/libtiff-build/include
    DEPENDPATH += $$PWD/../3rd-party/libtiff-build/include
    # inform user if these libs are missed
    !exists($$PWD/../3rd-party/zlib/zlib1.dll) | !exists($$PWD/../3rd-party/libtiff-build/tiff4.dll) {
        message("Seems like \"3rd-party\" folder is missed.")
        message("Check if https://github.com/polovik/3rd-party is cloned and built.")
        error("Some required libraries are missed.")
}
}
unix {
    LIBS += -lz -ltiff

    system(lupdate -verbose . -ts languages/lang_ru_RU.ts)
    system(lupdate -verbose . -ts languages/lang_en_US.ts)
    system(lrelease languages/lang_ru_RU.ts languages/lang_ru_RU.qm)
    system(lrelease languages/lang_en_US.ts languages/lang_en_US.qm)
    system(desktop-file-validate asa.desktop)
}

QMAKE_CXXFLAGS += -std=c++0x

win32 {
    RC_ICONS += icons/app_icon.ico
    QMAKE_TARGET_DESCRIPTION = "Analog signature analyzer"
    QMAKE_TARGET_COPYRIGHT = "Opensource"
    QMAKE_TARGET_PRODUCT = "Analog signature analyzer"
}

# DEPLOY
unix {
    isEmpty(PREFIX) {
        PREFIX = /usr
    }
    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share
#    first.commands = @echo ************Deploy application************ $$escape_expand(\n\t) \
#                     cp $${PWD}/icons/app_icon.png $${DESTDIR} $$escape_expand(\n\t) \
#                     cp $${PWD}/asa.desktop $${DESTDIR}
#    QMAKE_EXTRA_TARGETS += first

    target.path =$$BINDIR/
    desktop.path = $$DATADIR/applications/
    desktop.files += asa.desktop
    iconPixmap.path = $$DATADIR/pixmaps/
    iconPixmap.files += icons/asa.png
#    icon64.path = $$DATADIR/icons/hicolor/64x64/apps
#    icon64.files += ../data/64x64/$${TARGET}.png
    INSTALLS += target desktop iconPixmap
}
win32 {
    DESTDIR = ../build
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
    zlib_dll = $$PWD/../3rd-party/zlib/zlib1.dll
    zlib_dll = $$replace(zlib_dll, /, \\)
    tiff_dll = $$PWD/../3rd-party/libtiff-build/tiff4.dll
    tiff_dll = $$replace(tiff_dll, /, \\)
    system(xcopy /V /R /Y $$debug_bat $$resourcesTarget)
    system(xcopy /V /R /Y $$zlib_dll $$resourcesTarget)
    system(xcopy /V /R /Y $$tiff_dll $$resourcesTarget)

    # Qt libraries
    first.commands = @echo ************Copy libraries************ $$escape_expand(\n\t) \
                     windeployqt --no-translations $${DESTDIR} $$escape_expand(\n\t)
    QMAKE_EXTRA_TARGETS += first
#    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Core.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Gui.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5PrintSupport.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Multimedia.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5MultimediaWidgets.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Network.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5Widgets.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\Qt5OpenGL.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\libgcc_s_dw2-1.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\libwinpthread-1.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\libstdc++-6.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\bin\icu*.dll" $$resourcesTarget)
#    system(xcopy /V /R /Y "%QTDIR%\plugins\audio\qtaudio_windows.dll" $$resourcesTarget\audio\\)
#    system(xcopy /V /R /Y "%QTDIR%\plugins\platforms\qwindows.dll" $$resourcesTarget\platforms\\)
#    system(xcopy /V /R /Y "%QTDIR%\plugins\imageformats\qico.dll" $$resourcesTarget\imageformats\\)
#    system(xcopy /V /R /Y "%QTDIR%\plugins\printsupport\windowsprintersupport.dll" $$resourcesTarget\printsupport\\)
}
