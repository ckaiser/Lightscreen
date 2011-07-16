INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += $$PWD/AppUserModel.cpp \
           $$PWD/JumpList.cpp \
    $$PWD/TaskbarButton.cpp \
    $$PWD/Taskbar.cpp \
    $$PWD/TaskbarToolbar.cpp \
    $$PWD/Utils.cpp \
    $$PWD/TaskbarTabs.cpp

HEADERS += $$PWD/AppUserModel.h \
           $$PWD/JumpList.h \
           $$PWD/win7_include.h \
    $$PWD/TaskbarButton.h \
    $$PWD/Taskbar.h \
    $$PWD/TaskbarToolbar.h \
    $$PWD/TBPrivateData.h \
    $$PWD/JLPrivateData.h \
    $$PWD/Utils.h \
    $$PWD/TaskbarTabs.h

#LIBS += libuser32 libole32 libshell32 libshlwapi
