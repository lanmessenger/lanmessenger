// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "transferlistview.h"

lmcTransferListView::lmcTransferListView(QWidget* parent)
    : QListView(parent) {
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