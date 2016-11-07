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


#ifndef HISTORYTREEWIDGET_H
#define HISTORYTREEWIDGET_H

#include <QTreeWidget>
#include "uidefinitions.h"

class lmcHistoryTreeWidgetItem : public QTreeWidgetItem {
public:
	lmcHistoryTreeWidgetItem(QTreeWidget* parent = 0) : QTreeWidgetItem(parent) {}
	~lmcHistoryTreeWidgetItem() {}

	bool operator < (const QTreeWidgetItem& other) const {
		int column = treeWidget()->sortColumn();	
		if(column == 1)
			return data(column, IdRole).toDateTime() < other.data(column, DataRole).toDateTime();
		else
			return text(column).toLower() < other.text(column).toLower();
	}
};

#endif // HISTORYTREEWIDGET_H