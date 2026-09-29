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


#ifndef CHATWINDOW_H
#define CHATWINDOW_H

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
#include "ui_chatwindow.h"
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

class lmcChatWindow : public QWidget {
	Q_OBJECT

public:
    lmcChatWindow(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
	~lmcChatWindow(void);

	void init(User* pLocalUser, User* pRemoteUser, bool connected);
    void stop(void);
	void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void connectionStateChanged(bool connected);
	void settingsChanged(void);

	QString localId;
	QHash<QString, QString> peerIds;
	QString threadId;
	bool groupMode;

signals:
	void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void showHistory(void);
	void showTransfers(void);
	void closed(QString* lpszUserId);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent);
	void changeEvent(QEvent* pEvent);
	void closeEvent(QCloseEvent* pEvent);
	void dragEnterEvent(QDragEnterEvent* pEvent);
	void dropEvent(QDropEvent* pEvent);

private slots:
	void btnFont_clicked(void);
	void btnFontColor_clicked(void);
	void btnFile_clicked(void);
    void btnFolder_clicked(void);
	void btnSave_clicked(void);
	void btnHistory_clicked(void);
	void btnTransfers_clicked(void);
	void smileyAction_triggered(void);
	void log_sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void checkChatState(void);

private:
	void createSmileyMenu(void);
	void createToolBar(void);
	void setUIText(void);
	void sendMessage(void);
    void sendFile(QString* lpszFilePath);
    void sendFolder(QString* lpszFolderPath);
    void sendObject(MessageType type, QString* lpszPath);
	void encodeMessage(QString* lpszMessage);
    void processFileOp(XmlMessage* pMessage);
	void appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage);
	void updateFileMessage(FileMode mode, FileOp op, QString fileId);
	void showStatus(int flag, bool add);
	QString getWindowTitle(void);
	void setMessageFont(QFont& font);
	void setChatState(ChatState newChatState);

	QString peerId;
	QString localName;
	QHash<QString, QString> peerNames;
	QHash<QString, QString> peerStatuses;
    QHash<QString, uint> peerCaps;
	User* pLocalUser;
	QString lastUserId;

	Ui::ChatWindow ui;
	lmcSettings* pSettings;
	lmcMessageLog* pMessageLog;
	QAction* pFontAction;
	QAction* pFontColorAction;
	lmcToolButton* pbtnSmiley;
	QAction* pFileAction;
    QAction* pFolderAction;
	QAction* pSaveAction;
	QToolBar* pRightBar;
	QAction* pHistoryAction;
	QAction* pTransferAction;
	QMenu* pSmileyMenu;
	lmcImagePickerAction* pSmileyAction;
	int nSmiley;
	bool bConnected;
	int infoFlag;
	bool showSmiley;
	bool sendKeyMod;
    bool clearOnClose;
	lmcSoundPlayer* pSoundPlayer;
	QColor messageColor;
	ChatState chatState;
	qint64 keyStroke;
	qint64 snapKeyStroke;
	bool dataSaved;
};

#endif // CHATWINDOW_H
