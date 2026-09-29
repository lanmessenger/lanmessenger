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


#ifndef CHATROOMWINDOW_H
#define CHATROOMWINDOW_H

#include <QWidget>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QFile>
#include <QTextStream>
#include <qevent.h>
#include "ui_chatroomwindow.h"
#include "shared.h"
#include "settings.h"
#include "history.h"
#include "messagelog.h"
#include "subcontrols.h"
#include "imagepickeraction.h"
#include "soundplayer.h"
#include "chatdefinitions.h"
#include "chathelper.h"
#include "stdlocation.h"
#include "xmlmessage.h"
#include "theme.h"

class lmcChatRoomWindow : public QWidget
{
    Q_OBJECT

public:
	explicit lmcChatRoomWindow(QWidget *parent = 0);
	~lmcChatRoomWindow();

	void init(User* pLocalUser, bool connected, QString thread = QString());
	void show(void);
	void stop(void);
	void addUser(User* pUser);
	void updateUser(User* pUser);
	void removeUser(QString* lpszUserId);
	void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void connectionStateChanged(bool connected);
	void settingsChanged(void);
	void selectContacts(QStringList* selectedContacts);

	QString threadId;
	QHash<QString, QString> peerIds;

signals:
	void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void chatStarting(QString* lpszUserId);
    void contactsAdding(QStringList* excludeList);
	void closed(QString* lpszThreadId);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent);
	void changeEvent(QEvent* pEvent);
	void closeEvent(QCloseEvent* pEvent);

private slots:
	void userConversationAction_triggered(void);
	void userFileAction_triggered(void);
	void userInfoAction_triggered(void);
	void btnFont_clicked(void);
	void btnFontColor_clicked(void);
	void btnSave_clicked(void);
	void smileyAction_triggered(void);
	void addContactAction_triggered(void);
	void log_sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void tvUserList_itemActivated(QTreeWidgetItem* pItem, int column);
	void tvUserList_itemContextMenu(QTreeWidgetItem* pItem, QPoint& pos);

private:
	void createUserMenu(void);
	void createSmileyMenu(void);
	void createToolBar(void);
	void setUIText(void);
	void sendMessage(void);
	void encodeMessage(QString* lpszMessage);
	void appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage);
	void showStatus(int flag, bool add);
	QString getWindowTitle(void);
	void setMessageFont(QFont& font);
	void updateStatusImage(QTreeWidgetItem* pItem, QString* lpszStatus);
	QTreeWidgetItem* getUserItem(QString* lpszUserId);
	QTreeWidgetItem* getGroupItem(QString* lpszGroupId);
	void setUserAvatar(QString* lpszUserId, QString* lpszFilePath = 0);

	QString localId;
	QString localName;
	QHash<QString, QString> peerNames;
	User* pLocalUser;
	QString lastUserId;
	bool groupMode;

	Ui::ChatRoomWindow ui;
	lmcSettings* pSettings;
	lmcMessageLog* pMessageLog;
	QMenu* pUserMenu;
	QAction* userChatAction;
	QAction* userFileAction;
	QAction* userInfoAction;
	QAction* pFontAction;
	QAction* pFontColorAction;
	lmcToolButton* pbtnSmiley;
	QAction* pSaveAction;
	QMenu* pSmileyMenu;
	lmcImagePickerAction* pSmileyAction;
	QAction* addContactAction;
	int nSmiley;
	bool bConnected;
	int infoFlag;
	bool showSmiley;
	bool sendKeyMod;
	lmcSoundPlayer* pSoundPlayer;
	QColor messageColor;
	bool dataSaved;
	bool windowLoaded;
};

#endif // CHATROOMWINDOW_H
