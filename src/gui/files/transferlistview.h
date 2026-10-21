// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
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