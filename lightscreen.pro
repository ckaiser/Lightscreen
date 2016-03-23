TEMPLATE = app
TARGET = lightscreen
HEADERS += dialogs/areadialog.h \
    dialogs/historydialog.h \
    dialogs/namingdialog.h \
    dialogs/optionsdialog.h \
    dialogs/previewdialog.h \
    dialogs/updaterdialog.h \
    lightscreenwindow.h \
    tools/os.h \
    tools/screenshot.h \
    tools/screenshotmanager.h \
    tools/windowpicker.h \
    updater/updater.h \
    widgets/hotkeywidget.h \
    tools/qtsingleapplication/qtlockedfile.h \
    tools/qtsingleapplication/qtsinglecoreapplication.h \
    tools/uploader/imageuploader.h \
    tools/uploader/imguruploader.h \
    tools/uploader/uploader.h
SOURCES += dialogs/areadialog.cpp \
    dialogs/historydialog.cpp \
    dialogs/namingdialog.cpp \
    dialogs/optionsdialog.cpp \
    dialogs/previewdialog.cpp \
    dialogs/updaterdialog.cpp \
    lightscreenwindow.cpp \
    main.cpp \
    tools/os.cpp \
    tools/screenshot.cpp \
    tools/screenshotmanager.cpp \
    tools/windowpicker.cpp \
    updater/updater.cpp \
    widgets/hotkeywidget.cpp \
    tools/qtsingleapplication/qtlockedfile.cpp \
    tools/qtsingleapplication/qtlockedfile_unix.cpp \
    tools/qtsingleapplication/qtlockedfile_win.cpp \
    tools/qtsingleapplication/qtsinglecoreapplication.cpp \
    tools/uploader/imageuploader.cpp \
    tools/uploader/imguruploader.cpp \
    tools/uploader/uploader.cpp
FORMS += dialogs/historydialog.ui \
    dialogs/namingdialog.ui \
    dialogs/optionsdialog.ui \
    lightscreenwindow.ui
RESOURCES += lightscreen.qrc

RC_FILE += lightscreen.rc
CODECFORSRC = UTF-8
QT += core gui network sql multimedia

include($$PWD/tools/qtsingleapplication/qtsingleapplication.pri)
include($$PWD/tools/UGlobalHotkey/uglobalhotkey.pri)

windows {
    QT += winextras

    contains(QMAKE_CC, gcc){
        LIBS += libgdi32 libgcc libuser32 libole32 libshell32 libshlwapi libcomctl32
        QMAKE_CXXFLAGS = -Wextra -Wall -Wpointer-arith
    }
    contains(QMAKE_CC, cl){
        LIBS += gdi32.lib user32.lib ole32.lib shell32.lib shlwapi.lib comctl32.lib
    }

    CONFIG += embed_manifest_exe

    QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
    QMAKE_LFLAGS_WINDOWS += /MANIFESTUAC:level=\'asInvoker\'
    QMAKE_LFLAGS_CONSOLE = /SUBSYSTEM:CONSOLE,5.01

    DEFINES        += _ATL_XP_TARGETING
    QMAKE_CFLAGS   += /D _USING_V110_SDK71
    QMAKE_CXXFLAGS += /D _USING_V110_SDK71
}

unix:LIBS += -lX11
unix:QT += x11extras

OTHER_FILES += TODO.txt








