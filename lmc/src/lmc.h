/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
** 
** Contact:  dilipvrk@gmail.com
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
#include "userinfodialog.h"
#include "helpwindow.h"
#include "updatewindow.h"

class lmcCore : public QObject {
	Q_OBJECT

public:
	lmcCore(void);
	~lmcCore(void);
	void init(void);
	bool start(void);

public slots:
	bool receiveAppMessage(const QString& szMessage);

private slots:
	void exitApp(void);
	void aboutToExit(void);
	void timer_timeout(void);
	void startChat(QString* lpszUserId);
	void sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void connectionStateChanged(void);
	void showTransfers(void);
	void showHistory(void);
	void showSettings(void);
	void showHelp(QRect* pRect);
	void showUpdate(QRect* pRect);
	void historyCleared(void);
	void fileHistoryCleared(void);
	void showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle = QString::null, TrayMessageIcon icon = TMI_Info);
	void updateGroup(GroupOp op, QVariant value1, QVariant value2);

private:
	void stop(void);
	void loadSettings(void);
	void settingsChanged(void);
	void processMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
    void processFile(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void routeMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
    void createTransferWindow(void);
	void showTransferWindow(bool show = false);
	void initFileTransfer(FileMode mode, QString* lpszUserId, XmlMessage* pMessage);
	void showUserInfo(XmlMessage* pMessage);
	void createChatWindow(QString* lpszUserId);
	void showChatWindow(lmcChatWindow* chatWindow, bool show, bool alert = false);

	lmcSettings*			pSettings;
	QTimer*					pTimer;
	lmcMessaging*			pMessaging;
	lmcMainWindow*			pMainWindow;
	QList<lmcChatWindow*>	chatWindows;
	lmcTransferWindow*		pTransferWindow;
	lmcHistoryWindow*		pHistoryWindow;
	lmcSettingsDialog*		pSettingsDialog;
	lmcUserInfoDialog*		pUserInfoDialog;
	lmcHelpWindow*			pHelpWindow;
	lmcUpdateWindow*		pUpdateWindow;
	bool					messageTop;
	QString					lang;
};

#endif // LMC_H
