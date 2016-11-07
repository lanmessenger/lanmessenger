#-------------------------------------------------
#
# Project created by QtCreator 2011-08-29T16:07:59
#
#-------------------------------------------------

QT       += core gui network

TARGET = lmc
TEMPLATE = app


SOURCES += \
    lmc.cpp \
    mainwindow.cpp \
    main.cpp \
    usertreewidget.cpp \
    userinfodialog.cpp \
    transferwindow.cpp \
    transferlistview.cpp \
    tcpnetwork.cpp \
    soundplayer.cpp \
    settingsdialog.cpp \
    messaging.cpp \
    imagepickeraction.cpp \
    imagepicker.cpp \
    historywindow.cpp \
    chatwindow.cpp \
    history.cpp \
    broadcastwindow.cpp \
    filemodelview.cpp \
    datagram.cpp \
    aboutdialog.cpp \
    crypto.cpp \
    message.cpp \
    messagingproc.cpp \
    network.cpp \
    udpnetwork.cpp \
    netstreamer.cpp \
    settings.cpp \
    strings.cpp \
    helpwindow.cpp \
    shared.cpp

HEADERS  += \
    lmc.h \
    usertreewidget.h \
    userinfodialog.h \
    uidefinitions.h \
    transferwindow.h \
    transferlistview.h \
    toolbutton.h \
    tcpnetwork.h \
    soundplayer.h \
    shared.h \
    settingsdialog.h \
    settings.h \
    resource.h \
    messaging.h \
    mainwindow.h \
    imagepickeraction.h \
    imagepicker.h \
    historywindow.h \
    chatdefinitions.h \
    historytreewidget.h \
    history.h \
    broadcastwindow.h \
    filemodelview.h \
    definitions.h \
    datagram.h \
    chatwindow.h \
    aboutdialog.h \
    crypto.h \
    message.h \
    filetransfer.h \
    network.h \
    udpnetwork.h \
    netstreamer.h \
    strings.h \
    helpwindow.h

FORMS += \
    userinfodialog.ui \
    transferwindow.ui \
    settingsdialog.ui \
    mainwindow.ui \
    historywindow.ui \
    broadcastwindow.ui \
    chatwindow.ui \
    aboutdialog.ui \
    helpwindow.ui

RESOURCES +=

TRANSLATIONS += \
    lmc_en.ts \
    lmc_ml.ts

unix:!macx:!symbian: LIBS += -L$$PWD/../../lmcapp/lib/ -llmcapp

INCLUDEPATH += $$PWD/../../lmcapp/include
DEPENDPATH += $$PWD/../../lmcapp/include

unix:!macx:!symbian: LIBS += -L$$PWD/../../openssl/lib/ -lcrypto

INCLUDEPATH += $$PWD/../../openssl/include
DEPENDPATH += $$PWD/../../openssl/include

unix:!macx:!symbian: PRE_TARGETDEPS += $$PWD/../../openssl/lib/libcrypto.a
