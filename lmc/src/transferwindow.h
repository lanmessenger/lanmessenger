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


#ifndef TRANSFERWINDOW_H
#define TRANSFERWINDOW_H

#include <QWidget>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QDir>
#include <QToolBar>
#include <QToolButton>
#include <qevent.h>
#include "ui_transferwindow.h"
#include "shared.h"
#include "settings.h"
#include "stdlocation.h"
#include "soundplayer.h"
#include "xmlmessage.h"

class lmcTransferWindow : public QWidget
{
	Q_OBJECT

public:
	lmcTransferWindow(QWidget *parent = 0);
	~lmcTransferWindow(void);

	void init(void);
	void updateList(void);
	void stop(void);
    void createTransfer(MessageType type, FileMode mode, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage);
	void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void settingsChanged(void);

signals:
	void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
	void showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle, TrayMessageIcon icon);

protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent);
	void changeEvent(QEvent* pEvent);

private slots:
	void lvTransferList_currentRowChanged(int currentRow);
	void lvTransferList_activated(const QModelIndex& index);
	void btnCancel_clicked(void);
	void btnRemove_clicked(void);
	void btnClear_clicked(void);
	void btnShowFolder_clicked(void);
	void updateProgress(FileView* view, qint64 currentPos);

private:
	void createToolBar(void);
	void setUIText(void);
	void setButtonState(FileView::TransferState state);
	QPixmap getIcon(QString filePath);
	QString formatTime(qint64 size, qint64 speed);
	void clearList(void);

	Ui::TransferWindow ui;
	lmcSettings* pSettings;
	lmcSoundPlayer* pSoundPlayer;
	QAction* pactCancel;
	QAction* pactShowFolder;
	QAction* pactRemove;
	QList<FileView> pendingSendList;
};

#endif // TRANSFERWINDOW_H
