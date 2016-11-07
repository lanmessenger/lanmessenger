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


#include "transferlistview.h"

lmcTransferListView::lmcTransferListView(QWidget* parent) : QListView(parent) {
	pModel = new FileModel();
	setModel(pModel);
	setItemDelegate(new FileDelegate);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void lmcTransferListView::insertItem(int row, FileView* fileTransfer) {
	pModel->insertItem(row, fileTransfer);
}

void lmcTransferListView::removeItem(int row) {
	pModel->removeItem(row);
}

FileView* lmcTransferListView::item(int row) {
	return pModel->item(row);
}

FileView* lmcTransferListView::item(QString id) {
	return pModel->item(id);
}

FileView* lmcTransferListView::item(QString id, FileView::TransferMode mode) {
	return pModel->item(id, mode);
}

int lmcTransferListView::itemIndex(QString id, FileView::TransferMode mode) {
	return pModel->itemIndex(id, mode);
}

FileView* lmcTransferListView::currentItem(void) {
	return pModel->item(currentRow());
}

int lmcTransferListView::currentRow(void) {
	QModelIndex index = currentIndex();
	if(!index.isValid())
		return -1;
	if(index.row() >= pModel->rowCount())
		return -1;

	return index.row();
}

int lmcTransferListView::count(void) {
	return pModel->rowCount();
}

void lmcTransferListView::setCurrentRow(int row) {
	selectionModel()->setCurrentIndex(pModel->index(row), QItemSelectionModel::ClearAndSelect);
}

void lmcTransferListView::itemChanged(int row) {
	pModel->itemChanged(row);
}

void lmcTransferListView::currentChanged(const QModelIndex& current, const QModelIndex& previous) {
	QListView::currentChanged(current, previous);
	emit currentRowChanged(current.row());
}

void lmcTransferListView::loadData(QString filePath) {
	pModel->loadData(filePath);
}

void lmcTransferListView::saveData(QString filePath) {
	pModel->saveData(filePath);
}