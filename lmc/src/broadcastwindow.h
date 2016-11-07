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


#ifndef BROADCASTWINDOW_H
#define BROADCASTWINDOW_H

#include <QWidget>
#include <QToolBar>
#include <QMenu>
#include <qevent.h>
#include "ui_broadcastwindow.h"
#include "shared.h"
#include "settings.h"
#include "imagepickeraction.h"
#include "subcontrols.h"
#include "chatdefinitions.h"
#include "chathelper.h"
#include "xmlmessage.h"

class lmcBroadcastWindow : public QWidget
{
	Q_OBJECT

public:
	lmcBroadcastWindow(QWidget *parent = 0);
	~lmcBroadcastWindow();

	void init(bool connected);
	void stop(void);
	void show(QList<QTreeWidgetItem*>* pGroupList = 0);
	void connectionStateChanged(bool connected);
	void settingsChanged(void);

signals:
	void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);

protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent);
	void changeEvent(QEvent* pEvent);
	void closeEvent(QCloseEvent* pEvent);

private slots:
	void btnFontSize_clicked(void);
	void fontAction_triggered(QAction* action);
	void smileyAction_triggered(void);
	void btnSelectAll_clicked(void);
	void btnSelectNone_clicked(void);
	void tvUserList_itemChanged(QTreeWidgetItem* item, int column);
	void btnSend_clicked(void);

private:
	void createToolBar(void);
	void setUIText(void);
	void sendMessage(void);
	void encodeMessage(QString* lpszMessage);
	void showStatus(int flag, bool add);

	Ui::BroadcastWindow ui;
	lmcSettings* pSettings;
	QToolBar* pToolBar;
	QToolButton* pbtnFontSize;
	lmcToolButton* pbtnSmiley;
	int fontSizeVal;
	int nSmiley;
	bool bConnected;
	int infoFlag;
	bool showSmiley;
	bool sendKeyMod;
	bool parentToggling;
	bool childToggling;
	QActionGroup* pFontGroup;
};

#endif // BROADCASTWINDOW_H
