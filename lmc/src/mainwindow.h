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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QWidget>
#include <QSystemTrayIcon>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QIcon>
#include <qevent.h>
#include <QTreeWidget>
#include <QWidgetAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include "ui_mainwindow.h"
#include "shared.h"
#include "settings.h"
#include "imagepickeraction.h"
#include "aboutdialog.h"
#include "soundplayer.h"
#include "broadcastwindow.h"
#include "stdlocation.h"
#include "xmlmessage.h"

class lmcMainWindow : public QWidget {
	Q_OBJECT

public:
	lmcMainWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
	~lmcMainWindow(void);

	void init(User* pLocalUser, QList<QString>* pGroupList, bool connected);
	void start(void);
	void restore(void);
	void stop(void);
	void addUser(User* pUser);
	void updateUser(User* pUser);
	void removeUser(QString* lpszUserId);
	void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void connectionStateChanged(bool connected);
	void settingsChanged(bool init = false);
	void showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle = QString::null, TrayMessageIcon icon = TMI_Info);

signals:
	void appExiting(void);
	void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void chatStarting(QString* lpszUserId);
	void showTransfers(void);
	void showHistory(void);
	void showSettings(void);
	void showHelp(QRect* pRect);
	void showUpdate(QRect* pRect);
	void groupUpdated(GroupOp op, QVariant value1, QVariant value2);

protected:
	void closeEvent(QCloseEvent* pEvent);
	void changeEvent(QEvent* pEvent);

private slots:
	void sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void trayShowAction_triggered(void);
	void trayHistoryAction_triggered(void);
	void trayFileAction_triggered(void);
	void traySettingsAction_triggered(void);
	void trayAboutAction_triggered(void);
	void trayExitAction_triggered(void);
	void statusAction_triggered(QAction* action);
	void avatarAction_triggered(void);
	void avatarBrowseAction_triggered(void);
	void helpAction_triggered(void);
	void homePageAction_triggered(void);
	void updateAction_triggered(void);
	void refreshAction_triggered(void);
	void trayIcon_activated(QSystemTrayIcon::ActivationReason reason);
	void trayMessage_clicked(void);
	void tvUserList_itemActivated(QTreeWidgetItem* pItem, int column);
    void tvUserList_itemContextMenu(QTreeWidgetItem* pItem, QPoint& pos);
	void tvUserList_itemDragDropped(QTreeWidgetItem* pItem);
	void tvUserList_currentItemChanged(QTreeWidgetItem* pCurrent, QTreeWidgetItem* pPrevious);
	void groupAddAction_triggered(void);
	void groupRenameAction_triggered(void);
	void groupDeleteAction_triggered(void);
	void userConversationAction_triggered(void);
	void userBroadcastAction_triggered(void);
	void userFileAction_triggered(void);
	void userInfoAction_triggered(void);

private:
	void createMainMenu(void);
	void createTrayMenu(void);
	void createTrayIcon(void);
	void createStatusMenu(void);
	void createAvatarMenu(void);
	void createGroupMenu(void);
	void createUserMenu(void);
	void createToolBar(void);
	void setUIText(void);
	void showMinimizeMessage(void);
	void initGroups(QList<QString>* pGroupList);
	void updateStatusImage(QTreeWidgetItem* pItem, QString* lpszStatus);
	void setAvatar(QString fileName = QString());
	QTreeWidgetItem* getUserItem(QString* lpszUserId);
	QTreeWidgetItem* getGroupItem(QString* lpszGroupName);
	void sendMessage(MessageType type, QString* lpszUserId, QString* lpszMessage);
	void sendAvatar(QString* lpszUserId);
	void setUserAvatar(QString* lpszUserId);

	Ui::MainWindow ui;
	lmcSettings* pSettings;
	QMenuBar* pMainMenu;
	QSystemTrayIcon* pTrayIcon;
	QMenu* pFileMenu;
	QMenu* pToolsMenu;
	QMenu* pTrayMenu;
	QMenu* pHelpMenu;
	QMenu* pStatusMenu;
	QMenu* pAvatarMenu;
	QMenu* pGroupMenu;
	QMenu* pUserMenu;
	QAction* toolChatAction;
	QAction* toolFileAction;
	QAction* toolBroadcastAction;
	User* pLocalUser;
	bool bConnected;
	int nAvatar;
	bool showSysTray;
	bool showSysTrayMsg;
	bool showMinimizeMsg;
	bool minimizeHide;
	bool singleClickActivation;
	bool showAlert;
	bool noBusyAlert;
	bool noDNDAlert;
	lmcSoundPlayer* pSoundPlayer;
	TrayMessageType lastTrayMessageType;
	lmcBroadcastWindow* pBroadcastWindow;
	lmcAboutDialog* pAboutDialog;
	QActionGroup* statusGroup;
	QAction* refreshAction;
	QAction* exitAction;
	QAction* historyAction;
	QAction* transferAction;
	QAction* settingsAction;
	QAction* helpAction;
	QAction* onlineAction;
	QAction* updateAction;
	QAction* aboutAction;
	QAction* trayShowAction;
	QAction* trayStatusAction;
	QAction* trayHistoryAction;
	QAction* trayTransferAction;
	QAction* traySettingsAction;
	QAction* trayAboutAction;
	QAction* trayExitAction;
	QAction* groupAddAction;
	QAction* groupRenameAction;
	QAction* groupDeleteAction;
	QAction* userChatAction;
	QAction* userBroadcastAction;
	QAction* userFileAction;
	QAction* userInfoAction;
	QAction* avatarBrowseAction;
};

#endif // MAINWINDOW_H
