#-------------------------------------------------
#
# LAN Messenger Application project file
#
#-------------------------------------------------

QT       += core network widgets

TARGET = lmcapp
TEMPLATE = lib
VERSION = 2.0.0

DEFINES += LMCAPP_LIBRARY

SOURCES += \
    qtsinglecoreapplication.cpp \
    qtsingleapplication.cpp \
    qtlockedfile_win.cpp \
    qtlockedfile_unix.cpp \
    qtlockedfile.cpp \
    qtlocalpeer.cpp \
    application.cpp

HEADERS += \
    qtsinglecoreapplication.h \
    qtsingleapplication.h \
    qtlockedfile.h \
    qtlocalpeer.h \
    application.h \
    ../../lmc/src/settings.h

win32: RC_FILE = lmcappwin32.rc

symbian {
    #Symbian specific definitions
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE3FF4C6B
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = lmcapp.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}

win32: {
    CONFIG -= debug_and_release debug_and_release_target
    CONFIG += skip_target_version_ext
}
unix:!symbian {
    CONFIG(debug, debug|release) {
            DESTDIR = ../lib
            mac: TARGET = $$join(TARGET,,,_debug)
    } else {
            DESTDIR = ../lib
    }
}
