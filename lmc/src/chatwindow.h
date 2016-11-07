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


#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QtGui/QWidget>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QFileDialog>
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

class lmcChatWindow : public QWidget {
	Q_OBJECT

public:
	enum OutputFormat{ HtmlFormat, TextFormat };

	lmcChatWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
	~lmcChatWindow(void);

	void init(User* pLocalUser, User* pRemoteUser, bool connected);
	void stop(void);
	void receiveMessage(MessageType type, QString* lpszUserId, QString* lpszMesage);
	void connectionStateChanged(bool connected);
	void settingsChanged(void);

	QString localUserId;
	QString remoteUserId;

signals:
	void messageSent(MessageType type, QString* lpszUserId, QString* lpszMessage);
	void showHistory(void);
	void showTransfers(void);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent);
	void changeEvent(QEvent* pEvent);
	void dragEnterEvent(QDragEnterEvent* pEvent);
	void dropEvent(QDropEvent* pEvent);

private slots:
	void btnFontSize_clicked(void);
	void btnFile_clicked(void);
	void btnSave_clicked(void);
	void btnHistory_clicked(void);
	void btnTransfers_clicked(void);
	void fontAction_triggered(QAction* pAction);
	void smileyAction_triggered(void);
	void anchorClicked(const QUrl& link);

private:
	void createFontMenu(void);
	void createSmileyMenu(void);
	void createToolBar(void);
	void setUIText(void);
	void sendMessage(void);
	void sendFile(QString* lpszFilePath);
	void encodeMessage(QString* lpszMessage);
	void decodeMessage(QString* lpszMessage);
    void processFileOp(QString* lpszMessage);
	void updateMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, int nAvatar);
	void updateMessageLog(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, int nAvatar);
	void showInfoMessage(MessageType type, QString* lpszUserName, QString* lpszMessage);
	void updateInfoMessage(int position, const QString szMessage);
	void showStatus(int flag, bool add);
	QString prepareMessageLogForSave(OutputFormat format = HtmlFormat);
	void fileOperation(QString id, QString action);
	QString htmlHeader(void);

	QString localUserName;
	QString remoteUserName;
	QString remoteUserStatus;
	int remoteUserAvatar;
	User* pLocalUser;
	QString lastUserId;

	Ui::ChatWindow ui;
	lmcSettings* pSettings;
	QToolBar *pToolBar;
	QToolButton* pbtnFontSize;
	lmcToolButton* pbtnSmiley;
	QAction* pFileAction;
	QAction* pSaveAction;
	QAction* pHistoryAction;
	QAction* pTransferAction;
	QMenu* pFontMenu;
	QActionGroup* pFontGroup;
	QMenu* pSmileyMenu;
	lmcImagePickerAction* pSmileyAction;
	bool hasData;
	int fontSizeVal;
	int nSmiley;
	bool bConnected;
	int infoFlag;
	bool showSmiley;
	lmcSoundPlayer* pSoundPlayer;
	QMap<QString, QString> sendFileMap;
	QMap<QString, QString> receiveFileMap;
};

#endif // CHATWINDOW_H
