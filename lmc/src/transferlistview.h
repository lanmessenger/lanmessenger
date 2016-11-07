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


#ifndef TRANSFERLISTVIEW_H
#define TRANSFERLISTVIEW_H

#include <QListView>
#include <QPushButton>
#include "filemodelview.h"

class lmcTransferListView : public QListView {
	Q_OBJECT

public:
	lmcTransferListView(QWidget* parent = 0);
	~lmcTransferListView(void) {}

	void insertItem(int row, FileView* fileTransfer);
	void removeItem(int row);
	FileView* item(int row);
	FileView* item(QString id);
	FileView* item(QString id, FileView::TransferMode mode);
	int itemIndex(QString id, FileView::TransferMode mode);
	FileView* currentItem(void);
	int currentRow(void);
	int count(void);
	void setCurrentRow(int row);
	void itemChanged(int row);
	void loadData(QString filePath);
	void saveData(QString filePath);

signals:
	void currentRowChanged(int currentRow);

protected:
	void currentChanged(const QModelIndex& current, const QModelIndex& previous);

private:
	FileModel* pModel;
};

#endif // TRANSFERLISTVIEW_H