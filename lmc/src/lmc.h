/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
** 
** Contact:  qualiatech@gmail.com
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/


#ifndef LMC_H
#define LMC_H

#include <QObject>
#include <QTimer>
#include <QSysInfo>
#include <QPointer>
#include "shared.h"
#include "settings.h"
#include "messaging.h"
#include "mainwindow.h"
#include "chatwindow.h"
#include "history.h"
#include "stdlocation.h"
#include "transferwindow.h"
#include "historywindow.h"
#include "settingsdialog.h"
#include "userinfowindow.h"
#include "helpwindow.h"
#include "updatewindow.h"
#include "chatroomwindow.h"
#include "userselectdialog.h"
#include "aboutdialog.h"
#include "broadcastwindow.h"

class lmcCore : public QObject {
	Q_OBJECT

public:
	lmcCore(void);
	~lmcCore(void);
	void init(const QString& szCommandArgs);
	bool start(void);

public slots:
	bool receiveAppMessage(const QString& szMessage);

private slots:
	void exitApp(void);
	void aboutToExit(void);
	void timer_timeout(void);
	void startChat(QString* lpszUserId);
	void startChatRoom(QString* lpszThreadId);
	void sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void connectionStateChanged(void);
	void showTransfers(void);
	void showHistory(void);
	void showSettings(void);
	void showHelp(QRect* pRect);
	void showUpdate(QRect* pRect);
	void showAbout(void);
	void showBroadcast(void);
	void showPublicChat();
	void historyCleared(void);
	void fileHistoryCleared(void);
	void showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle = QString(), TrayMessageIcon icon = TMI_Info);
	void updateGroup(GroupOp op, QVariant value1, QVariant value2);
    void addContacts(QStringList *pExcludList);
	void chatWindow_closed(QString* lpszUserId);
	void chatRoomWindow_closed(QString* lpszThreadId);

private:
	void stop(void);
	void loadSettings(void);
	void settingsChanged(void);
	void processMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
    void processFile(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void routeMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void routeGroupMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void processPublicMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
    void createTransferWindow(void);
	void showTransferWindow(bool show = false);
    void initFileTransfer(MessageType type, FileMode mode, QString* lpszUserId, XmlMessage* pMessage);
	void showUserInfo(XmlMessage* pMessage);
	void createChatWindow(QString* lpszUserId);
	void showChatWindow(lmcChatWindow* chatWindow, bool show, bool alert = false);
	void createChatRoomWindow(QString* lpszThreadId);
	void showChatRoomWindow(lmcChatRoomWindow* chatRoomWindow, bool show, bool alert = false, bool add = false);
	void showPublicChatWindow(bool show, bool alert = false, bool open = false);
    QStringList showSelectContacts(QWidget* parent, uint caps, QStringList* excludeList);
	void showPortConflictMessage(void);

	lmcSettings*					pSettings;
	QTimer*							pTimer;
	lmcMessaging*					pMessaging;
	lmcMainWindow*					pMainWindow;
	QList<lmcChatWindow*>			chatWindows;
	QList<lmcChatRoomWindow*>		chatRoomWindows;
	lmcTransferWindow*				pTransferWindow;
	QPointer<lmcHistoryWindow>		pHistoryWindow;
	QPointer<lmcSettingsDialog>		pSettingsDialog;
	QPointer<lmcUserInfoWindow>		pUserInfoWindow;
	QPointer<lmcHelpWindow>			pHelpWindow;
	lmcUpdateWindow*				pUpdateWindow;
	QPointer<lmcChatRoomWindow>		pPublicChatWindow;
	QPointer<lmcUserSelectDialog>	pUserSelectDialog;
	QPointer<lmcAboutDialog>		pAboutDialog;
	QPointer<lmcBroadcastWindow>	pBroadcastWindow;
	bool							messageTop;
	bool							pubMessagePop;
	QString							lang;
	bool							adaptiveRefresh;
	int								refreshTime;
	XmlMessage*						pInitParams;
};

#endif // LMC_H
