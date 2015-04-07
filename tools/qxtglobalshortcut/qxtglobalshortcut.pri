!qws:!symbian {
    HEADERS  += $$PWD/qxtglobalshortcut.h
    HEADERS  += $$PWD/qxtglobalshortcut_p.h

    SOURCES  += $$PWD/qxtglobalshortcut.cpp

    unix:!macx {
        SOURCES += $$PWD/qxtglobalshortcut_x11.cpp
    }
    macx {
        SOURCES += $$PWD/qxtglobalshortcut_mac.cpp
    }
    win32 {
        SOURCES += $$PWD/qxtglobalshortcut_win.cpp
    }
}

