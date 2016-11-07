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


#include <QMessageBox>
#include <QTranslator>
#include "lmc.h"

lmcCore::lmcCore(void) {
	pMessaging = new lmcMessaging();
	connect(pMessaging, SIGNAL(messageReceived(MessageType, QString*, XmlMessage*)), 
		this, SLOT(receiveMessage(MessageType, QString*, XmlMessage*)));
	connect(pMessaging, SIGNAL(connectionStateChanged()), this, SLOT(connectionStateChanged()));
	pMainWindow = new lmcMainWindow();
	connect(pMainWindow, SIGNAL(appExiting()), this, SLOT(exitApp()));
	connect(pMainWindow, SIGNAL(chatStarting(QString*)), this, SLOT(startChat(QString*)));
	connect(pMainWindow, SIGNAL(messageSent(MessageType, QString*, XmlMessage*)), 
		this, SLOT(sendMessage(MessageType, QString*, XmlMessage*)));
	connect(pMainWindow, SIGNAL(showTransfers()), this, SLOT(showTransfers()));
	connect(pMainWindow, SIGNAL(showHistory()), this, SLOT(showHistory()));
	connect(pMainWindow, SIGNAL(showSettings()), this, SLOT(showSettings()));
	connect(pMainWindow, SIGNAL(showHelp(QRect*)), this, SLOT(showHelp(QRect*)));
	connect(pMainWindow, SIGNAL(showUpdate(QRect*)), this, SLOT(showUpdate(QRect*)));
	connect(pMainWindow, SIGNAL(groupUpdated(GroupOp, QVariant, QVariant)),
			this, SLOT(updateGroup(GroupOp, QVariant, QVariant)));
	chatWindows.clear();
	pTransferWindow = NULL;
	pHistoryWindow = NULL;
	pSettingsDialog = NULL;
	pUserInfoDialog = NULL;
	pHelpWindow = NULL;
	pUpdateWindow = NULL;
	pTimer = NULL;
}

lmcCore::~lmcCore(void) {
}

void lmcCore::init(void) {
	//	prevent auto app exit when last visible window is closed
	qApp->setQuitOnLastWindowClosed(false);

	loadSettings();

	pMessaging->init();
	pMainWindow->init(pMessaging->localUser, &pMessaging->groupList, pMessaging->isConnected());
}

bool lmcCore::start(void) {
	pMessaging->start();

	if(!pMessaging->canReceive()) {
		//	show message box, exit
		QMessageBox msgBox;
		msgBox.setWindowTitle(lmcStrings::appName());
		msgBox.setWindowIcon(QIcon(IDR_APPICON));
		msgBox.setIcon(QMessageBox::Critical);
		QString msg = tr("A port address conflict has been detected. %1 will close now.");
		msgBox.setText(msg.arg(lmcStrings::appName()));
		QString detail = tr("%1 cannot start because another application is using the port "\
			"configured for use with %2.");
		msgBox.setDetailedText(detail.arg(lmcStrings::appName(), lmcStrings::appName()));
		msgBox.exec();
		stop();
		return false;
	}

	pMainWindow->start();
	
	pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout(void)), this, SLOT(timer_timeout(void)));
	int refreshTime = pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt();
	pTimer->start(refreshTime * 1000);
	bool autoStart = pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool();
	lmcSettings::setAutoStart(autoStart);

	return true;
}

void lmcCore::loadSettings(void) {
	pSettings = new lmcSettings();
	lang = pSettings->value(IDS_LANGUAGE, IDS_LANGUAGE_VAL).toString();
	Application::setLanguage(lang);
	messageTop = pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool();
}

void lmcCore::settingsChanged(void) {
	pMessaging->settingsChanged();
	pMainWindow->settingsChanged();
	for(int index = 0; index < chatWindows.count(); index++)
		chatWindows[index]->settingsChanged();
	if(pTransferWindow)
		pTransferWindow->settingsChanged();
	if(pUserInfoDialog)
		pUserInfoDialog->settingsChanged();
	if(pHistoryWindow)
		pHistoryWindow->settingsChanged();
	if(pSettingsDialog)
		pSettingsDialog->settingsChanged();
	if(pHelpWindow)
		pHelpWindow->settingsChanged();

	messageTop = pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool();
	int refreshTime = pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt();
	pTimer->setInterval(refreshTime * 1000);
	bool autoStart = pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool();
	lmcSettings::setAutoStart(autoStart);
	QString appLang = pSettings->value(IDS_LANGUAGE, IDS_LANGUAGE_VAL).toString();
	if(appLang.compare(lang) != 0) {
		lang = appLang;
		Application::setLanguage(lang);
		lmcStrings::retranslate();
	}
}

void lmcCore::stop(void) {
	for(int index = 0; index < chatWindows.count(); index++) {
		chatWindows[index]->stop();
		delete chatWindows[index];
	}

	if(pTransferWindow) {
		pTransferWindow->stop();
		delete pTransferWindow;
	}

	if(pHistoryWindow) {
		pHistoryWindow->stop();
		delete pHistoryWindow;
	}

	if(pUserInfoDialog)
		delete pUserInfoDialog;

	if(pHelpWindow) {
		pHelpWindow->stop();
		delete pHelpWindow;
	}

	if(pUpdateWindow) {
		pUpdateWindow->stop();
		delete pUpdateWindow;
	}

	if(pTimer)
		pTimer->stop();

	pMessaging->stop();
	pMainWindow->stop();
}

//	This slot handles the exit signal emitted by main window when the user
//	selects quit from the menu.
void lmcCore::exitApp(void) {
	qApp->quit();
}

//	This slot handles the signal emitted by QApplication when the application
//	quits either by user interaction or from operating system signal.
void lmcCore::aboutToExit(void) {
	stop();
	pSettings->sync();
}

void lmcCore::timer_timeout(void) {
	pMessaging->update();
}

void lmcCore::startChat(QString* lpszUserId) {
	//	return if a chat window is already open for this user
	for(int index = 0; index < chatWindows.count(); index++)
		if(!chatWindows[index]->groupMode && chatWindows[index]->peerIds.contains(*lpszUserId)) {
			showChatWindow(chatWindows[index], true);
			return;
		}

	createChatWindow(lpszUserId);
	showChatWindow(chatWindows.last(), true);
}

void lmcCore::sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	QString data;

	switch(type) {
	case MT_Broadcast:
	case MT_Status:
	case MT_UserName:
	case MT_Message:
	case MT_GroupMessage:
	case MT_Query:
	case MT_Group:
	case MT_ChatState:
	case MT_Version:
		pMessaging->sendMessage(type, lpszUserId, pMessage);
		break;
	case MT_Refresh:
		pMessaging->update();
		break;
	case MT_LocalFile:
		data = pMessage->data(XN_FILEOP);
		if(data == FileOpNames[FO_Request]) {
			// this request message was sent from main window, route to chat window
			routeMessage(type, lpszUserId, pMessage);
		} else if(data == FileOpNames[FO_Accept]) { // accept message sent from chat window
			// init file transfer, transfer window will send message to messaging layer after validating filepath
			initFileTransfer(FM_Receive, lpszUserId, pMessage);
		} else {
			// any other file op send from chat window, send to messaging layer after rebadging as file message
			pMessaging->sendMessage(MT_File, lpszUserId, pMessage);
		}
		break;
	case MT_File:
		data = pMessage->data(XN_FILEOP);
		if(data == FileOpNames[FO_Request]) {
			// this request message was sent from chat window
			initFileTransfer(FM_Send, lpszUserId, pMessage);
		}
		pMessaging->sendMessage(type, lpszUserId, pMessage);
		break;
	case MT_Avatar:
		pMessaging->sendMessage(type, lpszUserId, pMessage);
		break;
	case MT_LocalAvatar:
		routeMessage(type, lpszUserId, pMessage);
		break;
	default:
		break;
	}
}

void lmcCore::receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	processMessage(type, lpszUserId, pMessage);
}

bool lmcCore::receiveAppMessage(const QString& szMessage) {
	bool doNotExit = true;

	if(szMessage.isEmpty()) {
		pMainWindow->restore();
		return doNotExit;
	}

	QStringList messageList = szMessage.split(" ", QString::SkipEmptyParts);
	//	remove duplicates
	messageList = messageList.toSet().toList();

	if(messageList.contains("/loopback")) {
		if(messageList.contains("/new"))
			pMessaging->setLoopback(true);
	}
	if(messageList.contains("/nohistory")) {
		QFile::remove(History::historyFile());
		if(pHistoryWindow)
			pHistoryWindow->updateList();
	}
	if(messageList.contains("/nofilehistory")) {
		QFile::remove(StdLocation::transferHistory());
		if(pTransferWindow)
			pTransferWindow->updateList();
	}
	if(messageList.contains("/noconfig")) {
		QFile::remove(pSettings->fileName());
		pSettings->sync();
		settingsChanged();
	}
	if(messageList.contains("/sync")) {
		bool autoStart = pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool();
		lmcSettings::setAutoStart(autoStart);
	}
	if(messageList.contains("/unsync")) {
		lmcSettings::setAutoStart(false);
	}
	if(messageList.contains("/term")) {
		doNotExit = false;
		exitApp();
	}
	if(messageList.contains("/quit")) {
		doNotExit  = false;
	}

	return doNotExit;
}

void lmcCore::connectionStateChanged() {
	bool connected = pMessaging->isConnected();

	pMainWindow->connectionStateChanged(connected);
	for(int index = 0; index < chatWindows.count(); index++)
		chatWindows[index]->connectionStateChanged(connected);
}

void lmcCore::showTransfers(void) {
	createTransferWindow();
	showTransferWindow(true);
}

void lmcCore::showHistory(void) {
	if(!pHistoryWindow) {
		pHistoryWindow = new lmcHistoryWindow();
		pHistoryWindow->init();
	}
	
	//	if window is minimized it, restore it to previous state
	if(pHistoryWindow->windowState().testFlag(Qt::WindowMinimized))
		pHistoryWindow->setWindowState(pHistoryWindow->windowState() & ~Qt::WindowMinimized);
	pHistoryWindow->setWindowState(pHistoryWindow->windowState() | Qt::WindowActive);
	pHistoryWindow->raise();	// make window the top most window of the application
	pHistoryWindow->show();
	pHistoryWindow->activateWindow();	// bring window to foreground
}

void lmcCore::showSettings(void) {
	if(!pSettingsDialog) {
		pSettingsDialog = new lmcSettingsDialog(pMainWindow);
		connect(pSettingsDialog, SIGNAL(historyCleared()), this, SLOT(historyCleared()));
		connect(pSettingsDialog, SIGNAL(fileHistoryCleared()), this, SLOT(fileHistoryCleared()));
	}

	if(pSettingsDialog->exec())
		settingsChanged();
}

void lmcCore::showHelp(QRect* pRect) {
	if(!pHelpWindow) {
		pHelpWindow = new lmcHelpWindow(pRect);
		pHelpWindow->init();
	}

	//	if window is minimized it, restore it to previous state
	if(pHelpWindow->windowState().testFlag(Qt::WindowMinimized))
		pHelpWindow->setWindowState(pHelpWindow->windowState() & ~Qt::WindowMinimized);
	pHelpWindow->setWindowState(pHelpWindow->windowState() | Qt::WindowActive);
	pHelpWindow->raise();	// make window the top most window of the application
	pHelpWindow->show();
	pHelpWindow->activateWindow();	// bring window to foreground
}

void lmcCore::showUpdate(QRect* pRect) {
	if(!pUpdateWindow) {
		pUpdateWindow = new lmcUpdateWindow(pRect);
		connect(pUpdateWindow, SIGNAL(messageSent(MessageType, QString*, XmlMessage*)),
			this, SLOT(sendMessage(MessageType, QString*, XmlMessage*)));
		pUpdateWindow->init();
	}

	//	if window is minimized it, restore it to previous state
	if(pUpdateWindow->windowState().testFlag(Qt::WindowMinimized))
		pUpdateWindow->setWindowState(pUpdateWindow->windowState() & ~Qt::WindowMinimized);
	pUpdateWindow->setWindowState(pUpdateWindow->windowState() | Qt::WindowActive);
	pUpdateWindow->raise();	// make window the top most window of the application
	pUpdateWindow->show();
	pUpdateWindow->activateWindow();	// bring window to foreground
}

void lmcCore::historyCleared(void) {
	if(pHistoryWindow)
		pHistoryWindow->updateList();
}

void lmcCore::fileHistoryCleared(void) {
	if(pTransferWindow)
		pTransferWindow->updateList();
}

void lmcCore::showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle, TrayMessageIcon icon) {
	pMainWindow->showTrayMessage(type, szMessage, szTitle, icon);
}

void lmcCore::updateGroup(GroupOp op, QVariant value1, QVariant value2) {
	pMessaging->updateGroup(op, value1, value2);
}

void lmcCore::processMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	switch(type) {
	case MT_Announce:
		pMainWindow->addUser(pMessaging->getUser(lpszUserId));
		break;
	case MT_Depart:
		pMainWindow->removeUser(lpszUserId);
		break;
	case MT_Status:
	case MT_UserName:
		pMainWindow->updateUser(pMessaging->getUser(lpszUserId));
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_Avatar:
		pMainWindow->receiveMessage(type, lpszUserId, pMessage);
		break;
	case MT_Message:
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_Broadcast:
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_Failed:
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_Error:
		break;
	case MT_Query:
		showUserInfo(pMessage);
		break;
	case MT_ChatState:
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_File:
        processFile(type, lpszUserId, pMessage);
		break;
	case MT_Version:
	case MT_WebFailed:
		pUpdateWindow->receiveMessage(type, lpszUserId, pMessage);
		break;
    default:
        break;
	}
}

void lmcCore::processFile(MessageType type, QString *lpszUserId, XmlMessage* pMessage) {
	int fileOp = Helper::indexOf(FileOpNames, FO_Max, pMessage->data(XN_FILEOP));
	switch(fileOp) {
	case FO_Accept:
		showTransferWindow();
		break;
	}
	if(fileOp != FO_Request && pTransferWindow)
		pTransferWindow->receiveMessage(type, lpszUserId, pMessage);

	routeMessage(type, lpszUserId, pMessage);
}

void lmcCore::routeMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	bool windowExists = false;
	bool needsNotice = (type == MT_Message || type == MT_Broadcast || type == MT_Failed
		|| (type == MT_File && pMessage->data(XN_FILEOP) == FileOpNames[FO_Request])
		|| (type == MT_LocalFile && pMessage->data(XN_FILEOP) == FileOpNames[FO_Request])
		|| type == MT_GroupMessage);

	//	If no specific user is specified, send this message to all windows
	if(!lpszUserId) {
		for(int index = 0; index < chatWindows.count(); index++) {
			chatWindows[index]->receiveMessage(type, lpszUserId, pMessage);
		}
	} else {
		QString threadId = pMessage->data(XN_THREAD);
		
		switch(type) {
		case MT_LocalAvatar:
		case MT_Status:
		case MT_UserName:
			for(int index = 0; index < chatWindows.count(); index++)
				if(chatWindows[index]->peerIds.contains(*lpszUserId))
					chatWindows[index]->receiveMessage(type, lpszUserId, pMessage);		
			break;
		default:
			for(int index = 0; index < chatWindows.count(); index++) {
				if(chatWindows[index]->peerIds.contains(*lpszUserId) && chatWindows[index]->threadId == threadId) {
					chatWindows[index]->receiveMessage(type, lpszUserId, pMessage);
					if(needsNotice)
						showChatWindow(chatWindows[index], messageTop, needsNotice);
					windowExists = true;
					break;
				}
			}
			break;
		}
	}

	//	create a new window if no chat window with this user exists and the
	//	incoming message is of type that needs notice
	if(!windowExists && needsNotice) {
		createChatWindow(lpszUserId);
		chatWindows.last()->receiveMessage(type, lpszUserId, pMessage);
		if(needsNotice)
			showChatWindow(chatWindows.last(), messageTop, needsNotice);
	}
}

void lmcCore::createTransferWindow(void) {
    if(!pTransferWindow) {
        pTransferWindow = new lmcTransferWindow();
        connect(pTransferWindow, SIGNAL(messageSent(MessageType, QString*, XmlMessage*)),
            this, SLOT(sendMessage(MessageType, QString*, XmlMessage*)));
        connect(pTransferWindow, SIGNAL(showTrayMessage(TrayMessageType, QString, QString, TrayMessageIcon)),
            this, SLOT(showTrayMessage(TrayMessageType, QString, QString, TrayMessageIcon)));
        pTransferWindow->init();
    }
}

void lmcCore::showTransferWindow(bool show) {
	bool autoShow = pSettings->value(IDS_AUTOSHOWFILE, IDS_AUTOSHOWFILE_VAL).toBool();
	bool bringToForeground = pSettings->value(IDS_FILETOP, IDS_FILETOP_VAL).toBool();

	if((autoShow && bringToForeground) || show) {
		//	if window is minimized it, restore it to previous state
		if(pTransferWindow->windowState().testFlag(Qt::WindowMinimized))
			pTransferWindow->setWindowState(pTransferWindow->windowState() & ~Qt::WindowMinimized);
		pTransferWindow->setWindowState(pTransferWindow->windowState() | Qt::WindowActive);
		pTransferWindow->raise();	// make main window the top most window of the application
		pTransferWindow->show();
		pTransferWindow->activateWindow();	// bring window to foreground
	}
	else if(autoShow && !bringToForeground) {
		if(pTransferWindow->isHidden())
			pTransferWindow->setWindowState(pTransferWindow->windowState() | Qt::WindowMinimized);
		pTransferWindow->setWindowState(pTransferWindow->windowState() | Qt::WindowActive);
		pTransferWindow->show();
		qApp->alert(pTransferWindow);
	}
}

void lmcCore::initFileTransfer(FileMode mode, QString* lpszUserId, XmlMessage* pMessage) {
    createTransferWindow();
    if(mode == FM_Receive)
        showTransferWindow();
	
	User* pUser = pMessaging->getUser(lpszUserId);
	pTransferWindow->createTransfer(mode, lpszUserId, &pUser->name, pMessage);
}

void lmcCore::showUserInfo(XmlMessage* pMessage) {
	if(!pUserInfoDialog) {
		pUserInfoDialog = new lmcUserInfoDialog(pMainWindow);
		pUserInfoDialog->init();
	}

	pUserInfoDialog->setInfo(pMessage);
	pUserInfoDialog->show();
}

void lmcCore::createChatWindow(QString* lpszUserId) {
	//	create new chat window for this user
	lmcChatWindow* pChatWindow = new lmcChatWindow();
	chatWindows.append(pChatWindow);
	User* pLocalUser = pMessaging->localUser;
	User* pRemoteUser = pMessaging->getUser(lpszUserId);
	connect(pChatWindow, SIGNAL(messageSent(MessageType, QString*, XmlMessage*)), 
		this, SLOT(sendMessage(MessageType, QString*, XmlMessage*)));
	connect(pChatWindow, SIGNAL(showHistory()), this, SLOT(showHistory()));
	connect(pChatWindow, SIGNAL(showTransfers()), this, SLOT(showTransfers()));
	pChatWindow->init(pLocalUser, pRemoteUser, pMessaging->isConnected());
}

void lmcCore::showChatWindow(lmcChatWindow* chatWindow, bool show, bool alert) {
	if(show) {
		if(chatWindow->windowState().testFlag(Qt::WindowMinimized))
			chatWindow->setWindowState(chatWindow->windowState() & ~Qt::WindowMinimized);
		chatWindow->show();
		chatWindow->activateWindow();
	} else {
		if(chatWindow->isHidden())
			chatWindow->setWindowState(chatWindow->windowState() | Qt::WindowMinimized);
		chatWindow->show();
		if(alert)
			qApp->alert(chatWindow);
	}
}
