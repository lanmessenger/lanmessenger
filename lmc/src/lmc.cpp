/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
** 
** Contact:  dilipvradhakrishnan@gmail.com
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
	connect(pMessaging, SIGNAL(messageReceived(MessageType, QString*, QString*)), 
		this, SLOT(receiveMessage(MessageType, QString*, QString*)));
	connect(pMessaging, SIGNAL(connectionStateChanged()), this, SLOT(connectionStateChanged()));
	pMainWindow = new lmcMainWindow();
	connect(pMainWindow, SIGNAL(appExiting()), this, SLOT(exitApp()));
	connect(pMainWindow, SIGNAL(chatStarting(QString*)), this, SLOT(startChat(QString*)));
	connect(pMainWindow, SIGNAL(messageSent(MessageType, QString*, QString*)), this, SLOT(sendMessage(MessageType, QString*, QString*)));
	connect(pMainWindow, SIGNAL(showTransfers()), this, SLOT(showTransfers()));
	connect(pMainWindow, SIGNAL(showHistory()), this, SLOT(showHistory()));
	connect(pMainWindow, SIGNAL(showSettings()), this, SLOT(showSettings()));
	connect(pMainWindow, SIGNAL(showHelp()), this, SLOT(showHelp()));
	chatWindows.clear();
	pTransferWindow = NULL;
	pHistoryWindow = NULL;
	pSettingsDialog = NULL;
	pUserInfoDialog = NULL;
	pHelpWindow = NULL;
	pTimer = NULL;
}

lmcCore::~lmcCore(void) {
}

void lmcCore::init(void) {
	//	prevent auto app exit when last visible window is closed
	qApp->setQuitOnLastWindowClosed(false);

	QApplication::setApplicationName(IDA_TITLE);
	QApplication::setOrganizationName(IDA_COMPANY);

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
		if(chatWindows[index]->remoteUserId.compare(lpszUserId) == 0) {
			showChatWindow(chatWindows[index], true);
			return;
		}

	createChatWindow(lpszUserId);
	showChatWindow(chatWindows.last(), true);
}

void lmcCore::sendMessage(MessageType type, QString* lpszUserId, QString* lpszData) {
	QString fileData;
	QFileInfo fileInfo;
	int fileOp;

	switch(type) {
	case MT_Refresh:
		pMessaging->update();
		break;
	case MT_LocalFileReq:	// this message was sent from main window
		routeMessage(type, lpszUserId, lpszData);
		break;
	case MT_LocalFileOp:	// this message was sent from chat window
		fileOp = lpszData->split(DELIMITER, QString::SkipEmptyParts)[0].toInt();
		switch(fileOp) {
		case FO_Accept:
			initFileTransfer(FM_Receive, lpszUserId, lpszData);
			// transfer window will then send a message to messaging layer after some processing
			break;
		default:
			pMessaging->sendMessage(MT_FileOp, lpszUserId, lpszData);
			break;
		}
		break;
	case MT_FileReq:
		initFileTransfer(FM_Send, lpszUserId, lpszData);
		pMessaging->sendMessage(type, lpszUserId, lpszData);
		break;
	default:
		pMessaging->sendMessage(type, lpszUserId, lpszData);
		break;
	}
}

void lmcCore::receiveMessage(MessageType type, QString* lpszUserId, QString* lpszData) {
	processMessage(type, lpszUserId, lpszData);
}

bool lmcCore::receiveAppMessage(const QString& szMessage) {
	bool doNotExit = true;

	if(szMessage.isEmpty()) {
		pMainWindow->restore();
		return doNotExit;
	}

	QStringList messageList = szMessage.split(" ", QString::SkipEmptyParts);

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
		QFile::remove(FileTransfer::historyFile());
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
	pHistoryWindow->raise();	// make main window the top most window of the application
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

void lmcCore::showHelp(void) {
	if(!pHelpWindow) {
		pHelpWindow = new lmcHelpWindow();
		pHelpWindow->init();
	}

	//	if window is minimized it, restore it to previous state
	if(pHelpWindow->windowState().testFlag(Qt::WindowMinimized))
		pHelpWindow->setWindowState(pHelpWindow->windowState() & ~Qt::WindowMinimized);
	pHelpWindow->setWindowState(pHelpWindow->windowState() | Qt::WindowActive);
	pHelpWindow->raise();	// make main window the top most window of the application
	pHelpWindow->show();
	pHelpWindow->activateWindow();	// bring window to foreground
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

void lmcCore::processMessage(MessageType type, QString* lpszUserId, QString* lpszData) {
	QString data = QString::null;

	switch(type) {
	case MT_Online:
		pMainWindow->addUser(pMessaging->getUser(lpszUserId));
		break;
	case MT_Offline:
		pMainWindow->removeUser(lpszUserId);
		break;
	case MT_Status:
	case MT_UserName:
		pMainWindow->updateUser(pMessaging->getUser(lpszUserId));
		routeMessage(type, lpszUserId, lpszData);
		break;
	case MT_Avatar:
		routeMessage(type, lpszUserId, lpszData);
		break;
	case MT_Message:
		routeMessage(type, lpszUserId, lpszData);
		break;
	case MT_Broadcast:
		routeMessage(type, lpszUserId, lpszData);
		break;
	case MT_Failed:
		routeMessage(type, lpszUserId, lpszData);
		break;
	case MT_Error:
		break;
	case MT_UserInfo:
		showUserInfo(lpszData);
		break;
	case MT_UserAction:
		break;
	case MT_FileReq:
    case MT_FileOp:
        processFile(type, lpszUserId, lpszData);
		break;
    default:
        break;
	}
}

void lmcCore::processFile(MessageType type, QString *lpszUserId, QString *lpszData) {
    QStringList fileData = lpszData->split(DELIMITER, QString::SkipEmptyParts);

    int fileOp;
    switch(type) {
    case MT_FileOp:
        fileOp = fileData[FD_Op].toInt();
        switch(fileOp) {
        case FO_Accept:
            showTransferWindow();
            break;
        }
		if(pTransferWindow)
			pTransferWindow->receiveMessage(type, lpszUserId, lpszData);
        break;
    default:
        break;
    }

    routeMessage(type,lpszUserId, lpszData);
}

void lmcCore::routeMessage(MessageType type, QString* lpszUserId, QString* lpszData) {
	bool windowExists = false;
	bool needsNotice = (type == MT_Message || type == MT_Broadcast || type == MT_FileReq 
		|| type == MT_LocalFileReq || type == MT_Failed);

	for(int index = 0; index < chatWindows.count(); index++) {
		if(chatWindows[index]->remoteUserId.compare(lpszUserId) == 0) {
			chatWindows[index]->receiveMessage(type, lpszUserId, lpszData);
			if(needsNotice)
				showChatWindow(chatWindows[index], messageTop, needsNotice);
			windowExists = true;
			break;
		}
	}

	//	create a new window if no chat window with this user exists, and also the
	//	incoming message is of type 'Message', 'Broadcast', 'FileReq' or 'Failed'
	if(!windowExists && needsNotice) {
		createChatWindow(lpszUserId);
		chatWindows.last()->receiveMessage(type, lpszUserId, lpszData);
		if(needsNotice)
			showChatWindow(chatWindows.last(), messageTop, needsNotice);
	}
}

void lmcCore::createTransferWindow(void) {
    if(!pTransferWindow) {
        pTransferWindow = new lmcTransferWindow();
        connect(pTransferWindow, SIGNAL(messageSent(MessageType, QString*, QString*)),
            this, SLOT(sendMessage(MessageType, QString*, QString*)));
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

void lmcCore::initFileTransfer(FileMode mode, QString* lpszUserId, QString* lpszData) {
    createTransferWindow();
    if(mode == FM_Receive)
        showTransferWindow();
	
	User* pUser = pMessaging->getUser(lpszUserId);
	pTransferWindow->createTransfer(mode, lpszUserId, &pUser->name, lpszData);
}

void lmcCore::showUserInfo(QString* lpszUserInfo) {
	if(!pUserInfoDialog) {
		pUserInfoDialog = new lmcUserInfoDialog(pMainWindow);
		pUserInfoDialog->init();
	}

	pUserInfoDialog->setInfo(lpszUserInfo);
	pUserInfoDialog->show();
}

void lmcCore::createChatWindow(QString* lpszUserId) {
	//	create new chat window for this user
	lmcChatWindow* pChatWindow = new lmcChatWindow();
	chatWindows.append(pChatWindow);
	User* pLocalUser = pMessaging->localUser;
	User* pRemoteUser = pMessaging->getUser(lpszUserId);
	connect(pChatWindow, SIGNAL(messageSent(MessageType, QString*, QString*)), this, SLOT(sendMessage(MessageType, QString*, QString*)));
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
