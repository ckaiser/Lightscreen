TEMPLATE = app
TARGET = lightscreen
HEADERS += tools/os.h \
    updater/updater.h \
    dialogs/areadialog.h \
    dialogs/optionsdialog.h \
    widgets/hotkeywidget.h \
    lightscreenwindow.h \
    tools/screenshot.h \
    dialogs/previewdialog.h \
    tools/screenshotmanager.h \
    tools/windowpicker.h \
    tools/qtwin.h \
    dialogs/updaterdialog.h \
    dialogs/screenshotdialog.h \
    dialogs/namingdialog.h
SOURCES += tools/os.cpp \
    updater/updater.cpp \
    dialogs/areadialog.cpp \
    dialogs/optionsdialog.cpp \
    widgets/hotkeywidget.cpp \
    main.cpp \
    lightscreenwindow.cpp \
    tools/screenshot.cpp \
    dialogs/previewdialog.cpp \
    tools/screenshotmanager.cpp \
    tools/windowpicker.cpp \
    tools/qtwin.cpp \
    dialogs/updaterdialog.cpp \
    dialogs/screenshotdialog.cpp \
    dialogs/namingdialog.cpp
FORMS += dialogs/optionsdialog.ui \
    lightscreenwindow.ui \
    dialogs/namingdialog.ui
RESOURCES += lightscreen.qrc
TRANSLATIONS += translations/untranslated.ts \
                translations/spanish.ts \
                translations/russian.ts \
                translations/portugues.ts \
                translations/polish.ts \
                translations/japanese.ts \
                translations/italiano.ts \
                translations/dutch.ts
RC_FILE += lightscreen.rc
CODECFORSRC = UTF-8
LIBS += libgcc
QT += network \
    core \
    gui
win32:LIBS += libgdi32
include($$PWD/tools/globalshortcut/globalshortcut.pri)
include($$PWD/tools/qtsingleapplication/qtsingleapplication.pri)

OTHER_FILES += TODO.txt \
    TODO.txt
