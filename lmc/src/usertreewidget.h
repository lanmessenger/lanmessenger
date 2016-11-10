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


#ifndef USERTREEWIDGET_H
#define USERTREEWIDGET_H

#include <QtWidgets/QApplication>
#include <QtWidgets/QTreeWidget>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QString>
#include <QtWidgets/QStyledItemDelegate>
#include <QPainter>
#include "uidefinitions.h"

class lmcUserTreeWidgetItem : public QTreeWidgetItem {
public:
	lmcUserTreeWidgetItem();
	~lmcUserTreeWidgetItem() {}

	QRect checkBoxRect(const QRect& itemRect);
};

class lmcUserTreeWidgetGroupItem : public lmcUserTreeWidgetItem {
public:
	lmcUserTreeWidgetGroupItem() : lmcUserTreeWidgetItem() {}
	~lmcUserTreeWidgetGroupItem() {}

	void addChild(QTreeWidgetItem* child);
};

class lmcUserTreeWidgetUserItem : public lmcUserTreeWidgetItem {
public:
	lmcUserTreeWidgetUserItem() : lmcUserTreeWidgetItem() {}
	~lmcUserTreeWidgetUserItem() {}

private:
	bool operator < (const QTreeWidgetItem& other) const;
};

class lmcUserTreeWidgetDelegate : public QStyledItemDelegate {
public:
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
	void drawCheckBox(QPainter* painter, const QPalette& palette, const QRect& checkBoxRect, Qt::CheckState checkState) const;
};

class lmcUserTreeWidget : public QTreeWidget {
	Q_OBJECT

public:
	lmcUserTreeWidget(QWidget* parent);
	~lmcUserTreeWidget() {}

	bool checkable(void);
	void setCheckable(bool enable);
	UserListView view(void);
	void setView(UserListView view);

signals:
	void itemDragDropped(QTreeWidgetItem* item);
    void itemContextMenu(QTreeWidgetItem* item, QPoint& pos);

protected:
	void mousePressEvent(QMouseEvent* event);
	void dragMoveEvent(QDragMoveEvent* event);
	void dropEvent(QDropEvent* event);
    void contextMenuEvent(QContextMenuEvent *event);
	void mouseReleaseEvent(QMouseEvent* event);
	void keyPressEvent(QKeyEvent* event);

private:
	lmcUserTreeWidgetDelegate* itemDelegate;
	bool dragGroup;
	bool dragUser;
	QString parentId;
	QTreeWidgetItem* dragItem;
	bool expanded;
	bool isCheckable;
	UserListView viewType;
};

#endif // USERTREEWIDGET_H
