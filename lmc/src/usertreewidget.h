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


#ifndef USERTREEWIDGET_H
#define USERTREEWIDGET_H

#include <QTreeWidget>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QString>

class lmcUserTreeWidgetGroupItem : public QTreeWidgetItem {
public:
	lmcUserTreeWidgetGroupItem() : QTreeWidgetItem() {}
	~lmcUserTreeWidgetGroupItem() {};
};

class lmcUserTreeWidgetUserItem : public QTreeWidgetItem {
public:
	lmcUserTreeWidgetUserItem() : QTreeWidgetItem() {}
	~lmcUserTreeWidgetUserItem() {};
};

class lmcUserTreeWidget : public QTreeWidget {
	Q_OBJECT

public:
	lmcUserTreeWidget(QWidget* parent = 0) : QTreeWidget(parent) {}
	~lmcUserTreeWidget() {}

signals:
	void itemDragDropped(QTreeWidgetItem* item);
    void itemContextMenu(QTreeWidgetItem* item, QPoint& pos);

protected:
	void mousePressEvent(QMouseEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dropEvent(QDropEvent* event);
    void contextMenuEvent(QContextMenuEvent *event);

private:
	bool dragGroup;
	bool dragUser;
	QString parentId;
	QTreeWidgetItem* dragItem;
	bool expanded;
};

#endif // USERTREEWIDGET_H
