#-------------------------------------------------
#
# Project created by QtCreator 2011-08-29T16:00:09
#
#-------------------------------------------------

QT       += network

TARGET = lmcapp
TEMPLATE = lib

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
    application.h

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
