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


#ifndef HISTORYWINDOW_H
#define HISTORYWINDOW_H

#include <QWidget>
#include <QList>
#include <QTreeWidget>
#include "ui_historywindow.h"
#include "settings.h"
#include "history.h"
#include "historytreewidget.h"

class lmcHistoryWindow : public QWidget
{
	Q_OBJECT

public:
	lmcHistoryWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
	~lmcHistoryWindow();

	void init(void);
	void updateList(void);
	void stop(void);
	void settingsChanged(void);

protected:
	void changeEvent(QEvent* pEvent);

private slots:
	void tvMsgList_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
	void btnClearHistory_clicked(void);

private:
	void setUIText(void);
	void displayList(void);

	Ui::HistoryWindow ui;
	lmcSettings* pSettings;
	QList<MsgInfo> msgList;
};

#endif // HISTORYWINDOW_H
