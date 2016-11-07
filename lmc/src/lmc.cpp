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


#include <QMessageBox>
#include <QTranslator>
#include "trace.h"
#include "lmc.h"

lmcCore::lmcCore(void) {
	pMessaging = new lmcMessaging();
	connect(pMessaging, SIGNAL(messageReceived(MessageType, QString*, XmlMessage*)), 
		this, SLOT(receiveMessage(MessageType, QString*, XmlMessage*)));
	connect(pMessaging, SIGNAL(connectionStateChanged()), this, SLOT(connectionStateChanged()));
	pMainWindow = new lmcMainWindow();
	connect(pMainWindow, SIGNAL(appExiting()), this, SLOT(exitApp()));
	connect(pMainWindow, SIGNAL(chatStarting(QString*)), this, SLOT(startChat(QString*)));
	connect(pMainWindow, SIGNAL(chatRoomStarting(QString*)), this, SLOT(startChatRoom(QString*)));
	connect(pMainWindow, SIGNAL(messageSent(MessageType, QString*, XmlMessage*)), 
		this, SLOT(sendMessage(MessageType, QString*, XmlMessage*)));
	connect(pMainWindow, SIGNAL(showTransfers()), this, SLOT(showTransfers()));
	connect(pMainWindow, SIGNAL(showHistory()), this, SLOT(showHistory()));
	connect(pMainWindow, SIGNAL(showSettings()), this, SLOT(showSettings()));
	connect(pMainWindow, SIGNAL(showHelp(QRect*)), this, SLOT(showHelp(QRect*)));
	connect(pMainWindow, SIGNAL(showUpdate(QRect*)), this, SLOT(showUpdate(QRect*)));
	connect(pMainWindow, SIGNAL(showAbout()), this, SLOT(showAbout()));
	connect(pMainWindow, SIGNAL(showBroadcast()), this, SLOT(showBroadcast()));
	connect(pMainWindow, SIGNAL(showPublicChat()), this, SLOT(showPublicChat()));
	connect(pMainWindow, SIGNAL(groupUpdated(GroupOp, QVariant, QVariant)),
			this, SLOT(updateGroup(GroupOp, QVariant, QVariant)));
	pPublicChatWindow = new lmcChatRoomWindow();
	connect(pPublicChatWindow, SIGNAL(messageSent(MessageType, QString*, XmlMessage*)),
		this, SLOT(sendMessage(MessageType, QString*, XmlMessage*)));
	connect(pPublicChatWindow, SIGNAL(chatStarting(QString*)), this, SLOT(startChat(QString*)));
	chatWindows.clear();
	chatRoomWindows.clear();
	pTransferWindow = NULL;
	pHistoryWindow = NULL;
	pSettingsDialog = NULL;
	pUserInfoWindow = NULL;
	pHelpWindow = NULL;
	pUpdateWindow = NULL;
	pUserSelectDialog = NULL;
	pAboutDialog = NULL;
	pBroadcastWindow = NULL;
	pTimer = NULL;
}

lmcCore::~lmcCore(void) {
}

void lmcCore::init(const QString& szCommandArgs) {
	//	prevent auto app exit when last visible window is closed
	qApp->setQuitOnLastWindowClosed(false);

	QStringList arguments = szCommandArgs.split("\n", QString::SkipEmptyParts);
	//	remove duplicates
	arguments = arguments.toSet().toList();

	pInitParams = new XmlMessage();
	if(arguments.contains("/silent", Qt::CaseInsensitive))
		pInitParams->addData(XN_SILENTMODE, LMC_TRUE);
	if(arguments.contains("/trace", Qt::CaseInsensitive)) {
		pInitParams->addData(XN_TRACEMODE, LMC_TRUE);
		pInitParams->addData(XN_LOGFILE, StdLocation::freeLogFile());
	}
	for(int index = 0; index < arguments.count(); index++) {
		if(arguments.at(index).startsWith("/port=", Qt::CaseInsensitive)) {
			QString port = arguments.at(index).mid(QString("/port=").length());
			pInitParams->addData(XN_PORT, port);
			continue;
		}
		if(arguments.at(index).startsWith("/config=", Qt::CaseInsensitive)) {
			QString configFile = arguments.at(index).mid(QString("/config=").length());
			pInitParams->addData(XN_CONFIG, configFile);
			continue;
		}
	}

	lmcTrace::init(pInitParams);
	lmcTrace::write("Application initialized");

	loadSettings();

	lmcTrace::write("Settings loaded");

	pMessaging->init(pInitParams);
	pMainWindow->init(pMessaging->localUser, &pMessaging->groupList, pMessaging->isConnected());
	pPublicChatWindow->init(pMessaging->localUser, pMessaging->isConnected());
}

bool lmcCore::start(void) {
	lmcTrace::write("Application started");
	pMessaging->start();

	if(pMessaging->isConnected() && !pMessaging->canReceive()) {
		showPortConflictMessage();
		//	stop the application
		stop();
		return false;
	}

	pMainWindow->start();
	
	pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout(void)), this, SLOT(timer_timeout(void)));
	//	Set the timer to trigger 10 seconds after the application starts. After the
	//	first trigger, the timeout period will be decided by user settings.
	adaptiveRefresh = false;
	pTimer->start(10000);
	bool autoStart = pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool();
	lmcSettings::setAutoStart(autoStart);

	return true;
}

//	This is the initial point where settings are used in the application
void lmcCore::loadSettings(void) {
	pSettings = new lmcSettings();
	bool silent = Helper::stringToBool(pInitParams->data(XN_SILENTMODE));
	if(!pSettings->migrateSettings() && !silent) {
		// settings were reset. Show an alert if not in silent mode
		QString message = tr("Your preferences file is corrupt or invalid.\n\n%1 is unable to recover your settings.");
		QMessageBox::warning(NULL, lmcStrings::appName(), message.arg(lmcStrings::appName()));
	}
	if(pInitParams->dataExists(XN_CONFIG)) {
		QString configFile = pInitParams->data(XN_CONFIG);
		if(!pSettings->loadFromConfig(configFile) && !silent) {
			QString message = tr("Preferences could not be imported from '%1'.\n\n"\
								 "File may not exist, or may not be compatible with this version of %2.");
			QMessageBox::warning(NULL, lmcStrings::appName(), message.arg(configFile, lmcStrings::appName()));
		}
	}
	lang = pSettings->value(IDS_LANGUAGE, IDS_LANGUAGE_VAL).toString();
	Application::setLanguage(lang);
	Application::setLayoutDirection(tr("LAYOUT_DIRECTION") == RTL_LAYOUT ? Qt::RightToLeft : Qt::LeftToRight);
	messageTop = pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool();
	pubMessagePop = pSettings->value(IDS_PUBMESSAGEPOP, IDS_PUBMESSAGEPOP_VAL).toBool();
	refreshTime = pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt() * 1000;
}

void lmcCore::settingsChanged(void) {
	pMessaging->settingsChanged();
	pMainWindow->settingsChanged();
	if(pPublicChatWindow)
		pPublicChatWindow->settingsChanged();
	for(int index = 0; index < chatWindows.count(); index++)
		chatWindows[index]->settingsChanged();
	for(int index = 0; index < chatRoomWindows.count(); index++)
		chatRoomWindows[index]->settingsChanged();
	if(pTransferWindow)
		pTransferWindow->settingsChanged();
	if(pUserInfoWindow)
		pUserInfoWindow->settingsChanged();
	if(pHistoryWindow)
		pHistoryWindow->settingsChanged();
	if(pSettingsDialog)
		pSettingsDialog->settingsChanged();
	if(pHelpWindow)
		pHelpWindow->settingsChanged();
	if(pAboutDialog)
		pAboutDialog->settingsChanged();
	if(pBroadcastWindow)
		pBroadcastWindow->settingsChanged();

	messageTop = pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool();
	pubMessagePop = pSettings->value(IDS_PUBMESSAGEPOP, IDS_PUBMESSAGEPOP_VAL).toBool();
	refreshTime = pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt() * 1000;
	pTimer->setInterval(refreshTime);
	bool autoStart = pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool();
	lmcSettings::setAutoStart(autoStart);
	QString appLang = pSettings->value(IDS_LANGUAGE, IDS_LANGUAGE_VAL).toString();
	if(appLang.compare(lang) != 0) {
		lang = appLang;
		Application::setLanguage(lang);
		Application::setLayoutDirection(tr("LAYOUT_DIRECTION") == RTL_LAYOUT ? Qt::RightToLeft : Qt::LeftToRight);
		lmcStrings::retranslate();
	}
}

void lmcCore::stop(void) {
	for(int index = 0; index < chatWindows.count(); index++) {
		chatWindows[index]->stop();
		chatWindows[index]->deleteLater();
    }

	for(int index = 0; index < chatRoomWindows.count(); index++) {
		chatRoomWindows[index]->stop();
		chatRoomWindows[index]->deleteLater();
	}

	if(pTransferWindow) {
		pTransferWindow->stop();
		pTransferWindow->deleteLater();
	}

	if(pHistoryWindow) {
		pHistoryWindow->stop();
		pHistoryWindow->deleteLater();
	}

	if(pUserInfoWindow)
		pUserInfoWindow->deleteLater();

	if(pHelpWindow) {
		pHelpWindow->stop();
		pHelpWindow->deleteLater();
	}

	if(pUpdateWindow) {
		pUpdateWindow->stop();
		pUpdateWindow->deleteLater();
	}

	if(pBroadcastWindow) {
		pBroadcastWindow->stop();
		pBroadcastWindow->deleteLater();
	}

	if(pPublicChatWindow) {
		pPublicChatWindow->stop();
		pPublicChatWindow->deleteLater();
	}

	if(pTimer)
		pTimer->stop();

	pMessaging->stop();
	pMainWindow->stop();

	lmcTrace::write("Application stopped");
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
	pSettings->setValue(IDS_VERSION, IDA_VERSION);

	lmcTrace::write("Application exit");

	pSettings->sync();
}

void lmcCore::timer_timeout(void) {
	//	Refresh the contacts list whenever the timer triggers
	pMessaging->update();
	if(adaptiveRefresh) {
		//	The refresh interval is doubled until the refresh time defined by user is reached.
		//	Then keep refreshing at that interval.
		int nextInterval = pTimer->interval() * 2;
		int maxInterval = pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt() * 1000;
		int interval = qMin(nextInterval, maxInterval);
		adaptiveRefresh = (nextInterval >= maxInterval) ? false : true;
		pTimer->setInterval(interval);
	} else if(refreshTime > pTimer->interval()) {
		pTimer->setInterval(refreshTime);
	}
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

void lmcCore::startChatRoom(QString* lpszThreadId) {
	createChatRoomWindow(lpszThreadId);
	showChatRoomWindow(chatRoomWindows.last(), true, false, true);
}

void lmcCore::sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	QString data;

	switch(type) {
	case MT_Broadcast:
	case MT_UserName:
	case MT_Message:
	case MT_PublicMessage:
	case MT_GroupMessage:
	case MT_Query:
	case MT_Group:
	case MT_ChatState:
	case MT_Version:
    case MT_File:
    case MT_Avatar:
    case MT_Folder:
		pMessaging->sendMessage(type, lpszUserId, pMessage);
		break;
	case MT_Status:
	case MT_Note:
		pMessaging->sendMessage(type, lpszUserId, pMessage);
		processPublicMessage(type, lpszUserId, pMessage);
		routeGroupMessage(type, lpszUserId, pMessage);
		break;
	case MT_Refresh:
		pMessaging->update();
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

	QStringList messageList = szMessage.split("\n", QString::SkipEmptyParts);
	//	remove duplicates
	messageList = messageList.toSet().toList();

	if(messageList.contains("/new", Qt::CaseInsensitive)) {
		if(messageList.contains("/loopback", Qt::CaseInsensitive))
			pMessaging->setLoopback(true);
	}
	if(messageList.contains("/nohistory", Qt::CaseInsensitive)) {
		QFile::remove(History::historyFile());
		if(pHistoryWindow)
			pHistoryWindow->updateList();
	}
	if(messageList.contains("/nofilehistory", Qt::CaseInsensitive)) {
		QFile::remove(StdLocation::transferHistory());
		if(pTransferWindow)
			pTransferWindow->updateList();
	}
	if(messageList.contains("/noconfig", Qt::CaseInsensitive)) {
		QFile::remove(StdLocation::avatarFile());
		QFile::remove(pSettings->fileName());
		pSettings->sync();
		settingsChanged();
	}
	if(messageList.contains("/sync", Qt::CaseInsensitive)) {
		bool autoStart = pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool();
		lmcSettings::setAutoStart(autoStart);
	}
	if(messageList.contains("/unsync", Qt::CaseInsensitive)) {
		lmcSettings::setAutoStart(false);
	}
	if(messageList.contains("/term", Qt::CaseInsensitive)) {
		doNotExit = false;
		exitApp();
	}
	if(messageList.contains("/quit", Qt::CaseInsensitive)) {
		doNotExit  = false;
	}

	return doNotExit;
}

void lmcCore::connectionStateChanged(void) {
	bool connected = pMessaging->isConnected();

	pMainWindow->connectionStateChanged(connected);
	if(pPublicChatWindow)
		pPublicChatWindow->connectionStateChanged(connected);
	for(int index = 0; index < chatWindows.count(); index++)
		chatWindows[index]->connectionStateChanged(connected);
	for(int index = 0; index < chatRoomWindows.count(); index++)
		chatRoomWindows[index]->connectionStateChanged(connected);
	if(pBroadcastWindow)
		pBroadcastWindow->connectionStateChanged(connected);

	if(pMessaging->isConnected() && !pMessaging->canReceive()) {
		showPortConflictMessage();
		exitApp();
	}
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
		pSettingsDialog->init();
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

void lmcCore::showAbout(void) {
	if(!pAboutDialog) {
		pAboutDialog = new lmcAboutDialog(pMainWindow);
		pAboutDialog->init();
	}

	pAboutDialog->exec();
}

void lmcCore::showBroadcast(void) {
	if(!pBroadcastWindow) {
		pBroadcastWindow = new lmcBroadcastWindow();
		connect(pBroadcastWindow, SIGNAL(messageSent(MessageType, QString*, XmlMessage*)),
			this, SLOT(sendMessage(MessageType, QString*, XmlMessage*)));
		pBroadcastWindow->init(pMessaging->isConnected());
	}

	if(pBroadcastWindow->isHidden()) {
		QList<QTreeWidgetItem*> contactsList = pMainWindow->getContactsList();
		pBroadcastWindow->show(&contactsList);
	} else {
		pBroadcastWindow->show();
	}
}

void lmcCore::showPublicChat() {
	//	Show public chat window
	showPublicChatWindow(true);
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

void lmcCore::addContacts(QStringList* pExcludList) {
    lmcChatRoomWindow* chatRoomWindow = static_cast<lmcChatRoomWindow*>(sender());
    QStringList selectedContacts = showSelectContacts(chatRoomWindow, UC_GroupMessage, pExcludList);
	chatRoomWindow->selectContacts(&selectedContacts);
}

void lmcCore::chatWindow_closed(QString* lpszUserId) {
	for(int index = 0; index < chatWindows.count(); index++) {
		if(chatWindows[index]->peerIds.contains(*lpszUserId)) {
			chatWindows.removeAt(index);
			break;
		}
	}
}

void lmcCore::chatRoomWindow_closed(QString* lpszThreadId) {
	for(int index = 0; index < chatRoomWindows.count(); index++) {
		if(chatRoomWindows[index]->threadId.compare(*lpszThreadId) == 0) {
			chatRoomWindows.removeAt(index);
			break;
		}
	}
}

void lmcCore::processMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	switch(type) {
	case MT_Announce:
		pMainWindow->addUser(pMessaging->getUser(lpszUserId));
		processPublicMessage(type, lpszUserId, pMessage);
		break;
	case MT_Depart:
		pMainWindow->removeUser(lpszUserId);
		processPublicMessage(type, lpszUserId, pMessage);
        routeMessage(type, lpszUserId, pMessage);
		routeGroupMessage(type, lpszUserId, pMessage);
		break;
	case MT_Status:
	case MT_UserName:
	case MT_Note:
		pMainWindow->updateUser(pMessaging->getUser(lpszUserId));
		processPublicMessage(type, lpszUserId, pMessage);
		routeMessage(type, lpszUserId, pMessage);
		routeGroupMessage(type, lpszUserId, pMessage);
		break;
	case MT_Avatar:
        pMainWindow->receiveMessage(type, lpszUserId, pMessage);
        processPublicMessage(type, lpszUserId, pMessage);
        routeMessage(type, lpszUserId, pMessage);
        routeGroupMessage(type, lpszUserId, pMessage);
		break;
	case MT_Message:
		routeMessage(type, lpszUserId, pMessage);
		break;
	case MT_GroupMessage:
		routeGroupMessage(type, lpszUserId, pMessage);
		break;
	case MT_PublicMessage:
		processPublicMessage(type, lpszUserId, pMessage);
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
    case MT_Folder:
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
    int fileMode = Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));
	switch(fileOp) {
	case FO_Accept:
        initFileTransfer(type, (FileMode)fileMode, lpszUserId, pMessage);
        showTransferWindow();
		break;
    default:
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
        || (type == MT_Folder && pMessage->data(XN_FILEOP) == FileOpNames[FO_Request])
		|| type == MT_GroupMessage);

	//	If no specific user is specified, send this message to all windows
	if(!lpszUserId) {
		for(int index = 0; index < chatWindows.count(); index++) {
			chatWindows[index]->receiveMessage(type, lpszUserId, pMessage);
		}
	} else {
        QString threadId = pMessage ? pMessage->data(XN_THREAD) : QString::null;
		
		switch(type) {
        case MT_Avatar:
		case MT_Status:
		case MT_UserName:
			for(int index = 0; index < chatWindows.count(); index++)
                if(chatWindows[index]->peerIds.contains(*lpszUserId)
                        || chatWindows[index]->localId.compare(*lpszUserId) == 0)
					chatWindows[index]->receiveMessage(type, lpszUserId, pMessage);
			break;
		default:
			for(int index = 0; index < chatWindows.count(); index++) {
                if(chatWindows[index]->peerIds.contains(*lpszUserId)
                        && chatWindows[index]->threadId == threadId) {
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

void lmcCore::routeGroupMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	if(!lpszUserId) {
		for(int index = 0; index < chatRoomWindows.count(); index++) {
			if(type == MT_Status || type == MT_Note)
				chatRoomWindows[index]->updateUser(pMessaging->localUser);
			chatRoomWindows[index]->receiveMessage(type, lpszUserId, pMessage);
		}
	} else {
		QString threadId;
		int op;
		bool windowExists = false;

		switch(type) {
		case MT_Depart:
			// If pMessage is NULL, the user is actually offline. Remove user
			// from all chat rooms.
			if(!pMessage) {
				for(int index = 0; index < chatRoomWindows.count(); index++)
					if(chatRoomWindows[index]->peerIds.contains(*lpszUserId))
						chatRoomWindows[index]->removeUser(lpszUserId);
			}
			break;
		case MT_Status:
		case MT_UserName:
		case MT_Note:
			for(int index = 0; index < chatRoomWindows.count(); index++)
				if(chatRoomWindows[index]->peerIds.contains(*lpszUserId)) {
					chatRoomWindows[index]->updateUser(pMessaging->getUser(lpszUserId));
					chatRoomWindows[index]->receiveMessage(type, lpszUserId, pMessage);
				}
			break;
        case MT_Avatar:
			for(int index = 0; index < chatRoomWindows.count(); index++)
				if(chatRoomWindows[index]->peerIds.contains(*lpszUserId))
					chatRoomWindows[index]->receiveMessage(type, lpszUserId, pMessage);
			break;
		default:
			threadId = pMessage->data(XN_THREAD);
			op = Helper::indexOf(GroupMsgOpNames, GMO_Max, pMessage->data(XN_GROUPMSGOP));

			if(op == GMO_Request) {
				//	Check if a chat room with the thread id already exists
				for(int index = 0; index < chatRoomWindows.count(); index++) {
					if(chatRoomWindows[index]->threadId.compare(threadId) == 0) {
						chatRoomWindows[index]->addUser(pMessaging->localUser);
						windowExists = true;
						break;
					}
				}
				if(!windowExists) {
					createChatRoomWindow(&threadId);
					showChatRoomWindow(chatRoomWindows.last(), messageTop, true);
				}
			}
			for(int index = 0; index < chatRoomWindows.count(); index++) {
				if(chatRoomWindows[index]->threadId.compare(threadId) == 0) {
					switch(op) {
					case GMO_Request:
					case GMO_Join:
						chatRoomWindows[index]->addUser(pMessaging->getUser(lpszUserId));
						break;
					case GMO_Message:
						chatRoomWindows[index]->receiveMessage(type, lpszUserId, pMessage);
						showChatRoomWindow(chatRoomWindows[index], messageTop, true);
						break;
					case GMO_Leave:
						chatRoomWindows[index]->removeUser(lpszUserId);
						break;
					default:
						break;
					}
					break;
				}
			}
			break;
		}
	}
}

void lmcCore::processPublicMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	if(!pPublicChatWindow)
		return;

	switch(type) {
	case MT_Announce:
		pPublicChatWindow->addUser(pMessaging->getUser(lpszUserId));
		break;
	case MT_Depart:
		pPublicChatWindow->removeUser(lpszUserId);
		break;
	case MT_Status:
	case MT_UserName:
	case MT_Note:
		// lpszUserId can be NULL if sent by local user
		if(lpszUserId)
			pPublicChatWindow->updateUser(pMessaging->getUser(lpszUserId));
		else
			pPublicChatWindow->updateUser(pMessaging->localUser);
		break;
	case MT_PublicMessage:
		pPublicChatWindow->receiveMessage(type, lpszUserId, pMessage);
		showPublicChatWindow((pubMessagePop && messageTop), true, pubMessagePop);
		break;
    case MT_Avatar:
		pPublicChatWindow->receiveMessage(type, lpszUserId, pMessage);
		break;
	default:
		break;
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

void lmcCore::initFileTransfer(MessageType type, FileMode mode, QString *lpszUserId, XmlMessage *pMessage) {
    createTransferWindow();
	
	User* pUser = pMessaging->getUser(lpszUserId);
    pTransferWindow->createTransfer(type, mode, lpszUserId, &pUser->name, pMessage);
}

void lmcCore::showUserInfo(XmlMessage* pMessage) {
	if(!pUserInfoWindow) {
		pUserInfoWindow = new lmcUserInfoWindow();
		pUserInfoWindow->init();
	}

	pUserInfoWindow->setInfo(pMessage);

	//	if window is minimized it, restore it to previous state
	if(pUserInfoWindow->windowState().testFlag(Qt::WindowMinimized))
		pUserInfoWindow->setWindowState(pUserInfoWindow->windowState() & ~Qt::WindowMinimized);
	pUserInfoWindow->setWindowState(pUserInfoWindow->windowState() | Qt::WindowActive);
	pUserInfoWindow->raise();	// make window the top most window of the application
	pUserInfoWindow->show();
	pUserInfoWindow->activateWindow();	// bring window to foreground
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
	connect(pChatWindow, SIGNAL(closed(QString*)), this, SLOT(chatWindow_closed(QString*)));
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

void lmcCore::createChatRoomWindow(QString* lpszThreadId) {
	//	create a new chat room with the specified thread id
	lmcChatRoomWindow* pChatRoomWindow = new lmcChatRoomWindow();
	chatRoomWindows.append(pChatRoomWindow);
	User* pLocalUser = pMessaging->localUser;
	connect(pChatRoomWindow, SIGNAL(messageSent(MessageType, QString*, XmlMessage*)),
		this, SLOT(sendMessage(MessageType, QString*, XmlMessage*)));
    connect(pChatRoomWindow, SIGNAL(contactsAdding(QStringList*)),
        this, SLOT(addContacts(QStringList*)));
	connect(pChatRoomWindow, SIGNAL(chatStarting(QString*)), this, SLOT(startChat(QString*)));
	connect(pChatRoomWindow, SIGNAL(closed(QString*)), this, SLOT(chatRoomWindow_closed(QString*)));
	pChatRoomWindow->init(pLocalUser, pMessaging->isConnected(), *lpszThreadId);
}

void lmcCore::showChatRoomWindow(lmcChatRoomWindow* chatRoomWindow, bool show, bool alert, bool add) {
	// if show or add is specified, bring to top
	if(show || add) {
		if(chatRoomWindow->windowState().testFlag(Qt::WindowMinimized))
			chatRoomWindow->setWindowState(chatRoomWindow->windowState() & ~Qt::WindowMinimized);
		chatRoomWindow->show();
		chatRoomWindow->activateWindow();
	} else {
		if(chatRoomWindow->isHidden())
			chatRoomWindow->setWindowState(chatRoomWindow->windowState() | Qt::WindowMinimized);
		chatRoomWindow->show();
		if(alert)
			qApp->alert(chatRoomWindow);
	}

	// if add is specified, show Add Contact dialog
	// add should be specified only when the local user is creating a new chat room
	if(add) {
		QStringList excludeList(pMessaging->localUser->id);
        QStringList selectedContacts = showSelectContacts(chatRoomWindow, UC_GroupMessage, &excludeList);

		// if no contacts were selected, close the chat room window
		if(selectedContacts.count() == 0) {
			chatRoomWindows.removeOne(chatRoomWindow);
			chatRoomWindow->close();
		} else
			chatRoomWindow->selectContacts(&selectedContacts);
	}
}

void lmcCore::showPublicChatWindow(bool show, bool alert, bool open) {
	if(show) {
		if(pPublicChatWindow->windowState().testFlag(Qt::WindowMinimized))
			pPublicChatWindow->setWindowState(pPublicChatWindow->windowState() & ~Qt::WindowMinimized);
		pPublicChatWindow->show();
		pPublicChatWindow->activateWindow();
	} else {
		if(open) {
			if(pPublicChatWindow->isHidden())
				pPublicChatWindow->setWindowState(pPublicChatWindow->windowState() | Qt::WindowMinimized);
			pPublicChatWindow->show();
		}
		if(alert)
			qApp->alert(pPublicChatWindow);
	}
}

QStringList lmcCore::showSelectContacts(QWidget* parent, uint caps, QStringList* excludeList) {
	QStringList selectedContacts;
	pUserSelectDialog = new lmcUserSelectDialog(parent);

	QList<QTreeWidgetItem*> contactsList = pMainWindow->getContactsList();
	for(int index = 0; index < contactsList.count(); index++) {
		QTreeWidgetItem* pItem = contactsList.value(index);
		for(int childIndex = 0; childIndex < pItem->childCount(); childIndex++) {
			QTreeWidgetItem* pChildItem = pItem->child(childIndex);
			QString userId = pChildItem->data(0, IdRole).toString();
			User* pUser = pMessaging->getUser(&userId);
            if((pUser->caps & caps) != caps) {
				pItem->removeChild(pChildItem);
				childIndex--;
				continue;
			}
			if(excludeList->contains(userId)) {
				pItem->removeChild(pChildItem);
				childIndex--;
				continue;
			}
		}
	}

	pUserSelectDialog->init(&contactsList);
	if(pUserSelectDialog->exec())
		selectedContacts = pUserSelectDialog->selectedContacts;
	pUserSelectDialog->deleteLater();

	return selectedContacts;
}

void lmcCore::showPortConflictMessage(void) {
	//	show message box
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
}
