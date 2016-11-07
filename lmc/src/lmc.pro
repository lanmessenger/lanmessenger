#-------------------------------------------------
#
# LAN Messenger project file
#
#-------------------------------------------------

QT       += core gui network xml webkit

win32: TARGET = lmc
unix: TARGET = lan-messenger
macx: TARGET  = "LAN-Messenger"
TEMPLATE = app

SOURCES += \
    usertreewidget.cpp \
    userinfodialog.cpp \
    udpnetwork.cpp \
    transferwindow.cpp \
    transferlistview.cpp \
    tcpnetwork.cpp \
    strings.cpp \
    soundplayer.cpp \
    shared.cpp \
    settingsdialog.cpp \
    settings.cpp \
    network.cpp \
    netstreamer.cpp \
    messagingproc.cpp \
    messaging.cpp \
    message.cpp \
    mainwindow.cpp \
    main.cpp \
    lmc.cpp \
    imagepickeraction.cpp \
    imagepicker.cpp \
    historywindow.cpp \
    history.cpp \
    helpwindow.cpp \
    filemodelview.cpp \
    datagram.cpp \
    crypto.cpp \
    chatwindow.cpp \
    broadcastwindow.cpp \
	aboutdialog.cpp \
	xmlmessage.cpp \
    chathelper.cpp \
    theme.cpp \
    messagelog.cpp \
    updatewindow.cpp \
    webnetwork.cpp

HEADERS  += \
    usertreewidget.h \
    userinfodialog.h \
    uidefinitions.h \
    udpnetwork.h \
    transferwindow.h \
    transferlistview.h \
    toolbutton.h \
    tcpnetwork.h \
    strings.h \
    soundplayer.h \
    shared.h \
    settingsdialog.h \
    settings.h \
    resource.h \
    network.h \
    netstreamer.h \
    messaging.h \
    message.h \
    mainwindow.h \
    lmc.h \
    imagepickeraction.h \
    imagepicker.h \
    historywindow.h \
    historytreewidget.h \
    helpwindow.h \
    filemodelview.h \
    chatwindow.h \
    chatdefinitions.h \
    broadcastwindow.h \
    history.h \
	stdlocation.h \
    definitions.h \
    datagram.h \
    crypto.h \
	aboutdialog.h \
	xmlmessage.h \
    chathelper.h \
    theme.h \
    messagelog.h \
    updatewindow.h \
    webnetwork.h

FORMS += \
    userinfodialog.ui \
    transferwindow.ui \
    settingsdialog.ui \
    mainwindow.ui \
    historywindow.ui \
    helpwindow.ui \
    chatwindow.ui \
    broadcastwindow.ui \
    aboutdialog.ui \
    updatewindow.ui

TRANSLATIONS += \
	en_US.ts \
	ml_IN.ts \
	fr_FR.ts \
	de_DE.ts \
	tr_TR.ts \
	es_ES.ts

win32: RC_FILE = lmcwin32.rc
macx: ICON = lmc.icns

CONFIG(debug, debug|release) {
    DESTDIR = ../debug
} else {
    DESTDIR = ../release
}

win32: CONFIG(release, debug|release): LIBS += -L$$PWD/../../lmcapp/lib/ -llmcapp
else:win32: CONFIG(debug, debug|release): LIBS += -L$$PWD/../../lmcapp/lib/ -llmcappd
unix:!symbian: LIBS += -L$$PWD/../../lmcapp/lib/ -llmcapp

INCLUDEPATH += $$PWD/../../lmcapp/include
DEPENDPATH += $$PWD/../../lmcapp/include

win32: LIBS += -L$$PWD/../../openssl/lib/ -lcrypto
unix:!symbian: LIBS += -L$$PWD/../../openssl/lib/ -lcrypto

INCLUDEPATH += $$PWD/../../openssl/include
DEPENDPATH += $$PWD/../../openssl/include
