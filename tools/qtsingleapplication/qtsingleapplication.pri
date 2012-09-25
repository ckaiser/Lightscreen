INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
QT *= network

contains(QT_VERSION, ^5.*) {
    QT *= widgets
}

qtsingleapplication-uselib:!qtsingleapplication-buildlib {
    LIBS += -L$$QTSINGLEAPPLICATION_LIBDIR -l$$QTSINGLEAPPLICATION_LIBNAME
} else {
    SOURCES += $$PWD/qtsingleapplication.cpp $$PWD/qtlocalpeer.cpp
    HEADERS += $$PWD/qtsingleapplication.h $$PWD/qtlocalpeer.h
}

win32 {
    contains(TEMPLATE, lib):contains(CONFIG, shared):DEFINES += QT_QTSINGLEAPPLICATION_EXPORT
    else:qtsingleapplication-uselib:DEFINES += QT_QTSINGLEAPPLICATION_IMPORT
}
