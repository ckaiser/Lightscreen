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
    tools/qtimgur.h \
    tools/screenshot.h \
    tools/screenshotmanager.h \
    tools/uploader.h \
    tools/windowpicker.h \
    updater/updater.h \
    widgets/hotkeywidget.h
SOURCES += dialogs/areadialog.cpp \
    dialogs/historydialog.cpp \
    dialogs/namingdialog.cpp \
    dialogs/optionsdialog.cpp \
    dialogs/previewdialog.cpp \
    dialogs/updaterdialog.cpp \
    lightscreenwindow.cpp \
    main.cpp \
    tools/os.cpp \
    tools/qtimgur.cpp \
    tools/screenshot.cpp \
    tools/screenshotmanager.cpp \
    tools/uploader.cpp \
    tools/windowpicker.cpp \
    updater/updater.cpp \
    widgets/hotkeywidget.cpp
FORMS += dialogs/historydialog.ui \
    dialogs/namingdialog.ui \
    dialogs/optionsdialog.ui \
    lightscreenwindow.ui
RESOURCES += lightscreen.qrc

RC_FILE += lightscreen.rc
CODECFORSRC = UTF-8
QT += network core gui xml sql

include($$PWD/tools/globalshortcut/globalshortcut.pri)
include($$PWD/tools/qtsingleapplication/qtsingleapplication.pri)
include($$PWD/tools/qwin7utils/qwin7utils.pri)

windows{
    contains(QMAKE_CC, gcc){
        LIBS += libgdi32 libgcc libuser32 libole32 libshell32 libshlwapi libcomctl32
        QMAKE_CXXFLAGS = -Wextra -Wall -Wpointer-arith
    }
    contains(QMAKE_CC, cl){
        LIBS += gdi32.lib user32.lib ole32.lib shell32.lib shlwapi.lib comctl32.lib
    }
}

unix:LIBS += -lX11

OTHER_FILES += TODO.txt








