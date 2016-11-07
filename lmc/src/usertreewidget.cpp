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


#include "uidefinitions.h"
#include "usertreewidget.h"

bool lmcUserTreeWidgetUserItem::operator < (const QTreeWidgetItem& other) const {
	int column = treeWidget()->sortColumn();
	if(column == 0) {
		//	sort based on status and user name
		if(data(column, StatusRole).toInt() < other.data(column, StatusRole).toInt())
			return true;
		else if(data(column, StatusRole).toInt() > other.data(column, StatusRole).toInt())
			return false;
		else
			return text(column).toLower() < other.text(column).toLower();
	}

	return text(column).toLower() < other.text(column).toLower();
}

void lmcUserTreeWidget::mousePressEvent(QMouseEvent* event) {
	if(event->button() == Qt::LeftButton) {
		QTreeWidgetItem* item = itemAt(event->pos());

		dragGroup = false;
		dragUser = false;
		dragItem = NULL;
		parentId = QString::null;
		expanded = false;

		if(item) {
			dragItem = item;
			if(dynamic_cast<lmcUserTreeWidgetGroupItem*>(item)) {
				dragGroup = true;
				expanded = dragItem->isExpanded();
			}
			else if(dynamic_cast<lmcUserTreeWidgetUserItem*>(item)) {
				dragUser = true;
				parentId = dragItem->parent()->data(0, IdRole).toString();
			}
		}
	}

	QTreeWidget::mousePressEvent(event);
}

void lmcUserTreeWidget::dragMoveEvent(QDragMoveEvent* event) {
	QTreeWidget::dragMoveEvent(event);

	QTreeWidgetItem* item = itemAt(event->pos());
	bool accept = false;

	if(dragUser) {
		if(item && dynamic_cast<lmcUserTreeWidgetGroupItem*>(item) && visualItemRect(item).contains(event->pos(), true))
			accept = true;
	}
	else if(dragGroup) {
		if(!item || (dynamic_cast<lmcUserTreeWidgetGroupItem*>(item) && !visualItemRect(item).contains(event->pos(), true)))
			accept = true;
	}

	accept ? event->accept() : event->ignore();
}

void lmcUserTreeWidget::dropEvent(QDropEvent* event) {
	QTreeWidget::dropEvent(event);
		
	if(dragUser) {
		if(!dragItem->parent()) {
		//	user item dragged to group level. revert
			for(int index = 0; index < topLevelItemCount(); index++) {
				QTreeWidgetItem* parentItem = topLevelItem(index);
				if(parentItem->data(0, IdRole).toString().compare(parentId) == 0) {
					takeTopLevelItem(indexOfTopLevelItem(dragItem));
					parentItem->addChild(dragItem);
					return;
				}
			}
		}
	}
	else if(dragGroup) {
		dragItem->setExpanded(expanded);
	}

	if(dragItem) 
		emit itemDragDropped(dragItem);
}

void lmcUserTreeWidget::contextMenuEvent(QContextMenuEvent* event) {
    QTreeWidget::contextMenuEvent(event);

    QTreeWidgetItem* item = itemAt(event->pos());
    QPoint pos = event->globalPos();
    if(item && event->reason() != QContextMenuEvent::Mouse) {
        QRect itemRect = visualItemRect(item);
        pos = itemRect.bottomLeft();
        pos.ry() += itemRect.height();
        pos = mapToGlobal(pos);
    }

    emit itemContextMenu(item, pos);
}