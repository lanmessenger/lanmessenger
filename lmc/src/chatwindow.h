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


#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QtGui/QWidget>
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
#include "toolbutton.h"
#include "imagepickeraction.h"
#include "soundplayer.h"
#include "chatdefinitions.h"
#include "stdlocation.h"
#include "xmlmessage.h"

class lmcChatWindow : public QWidget {
	Q_OBJECT

public:
	enum OutputFormat{ HtmlFormat, TextFormat };

	lmcChatWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
	~lmcChatWindow(void);

	void init(User* pLocalUser, User* pRemoteUser, bool connected);
	void stop(void);
	void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void connectionStateChanged(bool connected);
	void settingsChanged(void);

	QString localUserId;
	QStringList remoteUserIds;
	QString threadId;
	bool groupMode;

signals:
	void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void showHistory(void);
	void showTransfers(void);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent);
	void changeEvent(QEvent* pEvent);
	void dragEnterEvent(QDragEnterEvent* pEvent);
	void dropEvent(QDropEvent* pEvent);

private slots:
	void btnFont_clicked(void);
	void btnFontColor_clicked(void);
	void btnFile_clicked(void);
	void btnSave_clicked(void);
	void btnHistory_clicked(void);
	void btnTransfers_clicked(void);
	void smileyAction_triggered(void);
	void anchorClicked(const QUrl& link);

private:
	void createSmileyMenu(void);
	void createToolBar(void);
	void setUIText(void);
	void sendMessage(void);
	void sendFile(QString* lpszFilePath);
	void encodeMessage(QString* lpszMessage);
	void decodeMessage(QString* lpszMessage);
    void processFileOp(XmlMessage* pMessage);
	void updateMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QFont* pFont, QColor* pColor);
	void updateMessageLog(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QFont* pFont, QColor* pColor);
	void showInfoMessage(MessageType type, QString* lpszUserName, XmlMessage* pMessage);
	void updateInfoMessage(int position, const QString szMessage);
	void updateUserName(QString* lpszUserId, QString* lpszUserName);
	void showStatus(int flag, bool add);
	QString prepareMessageLogForSave(OutputFormat format = HtmlFormat);
	void fileOperation(QString id, QString action);
	QString htmlHeader(void);
	QString getWindowTitle(void);
	void reloadMessageLog(void);
	QString getFontStyle(QFont* pFont, QColor* pColor, bool size = false);
	void setMessageFont(QFont& font);

	QString localUserName;
	QStringList remoteUserNames;
	QStringList remoteUserStatuses;
	QList<int> remoteUserAvatars;
	User* pLocalUser;
	QString lastUserId;

	Ui::ChatWindow ui;
	lmcSettings* pSettings;
	QToolBar *pToolBar;
	QAction* pFontAction;
	QAction* pFontColorAction;
	lmcToolButton* pbtnSmiley;
	QAction* pFileAction;
	QAction* pSaveAction;
	QAction* pHistoryAction;
	QAction* pTransferAction;
	QMenu* pSmileyMenu;
	lmcImagePickerAction* pSmileyAction;
	bool hasData;
	int fontSizeVal;
	int nSmiley;
	bool bConnected;
	int infoFlag;
	bool showSmiley;
	lmcSoundPlayer* pSoundPlayer;
	QMap<QString, XmlMessage> sendFileMap;
	QMap<QString, XmlMessage> receiveFileMap;
};

#endif // CHATWINDOW_H
