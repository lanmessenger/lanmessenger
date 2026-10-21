// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QTableWidget>

class lmcImagePicker : public QTableWidget {
  Q_OBJECT

public:
  lmcImagePicker(QWidget* parent, QList<QString>* source, int picSize, int columns, int* selected, int actionIndex);
  ~lmcImagePicker();

protected:
  void currentChanged(const QModelIndex& current, const QModelIndex& preious);
  void mouseMoveEvent(QMouseEvent* e);
  void paintEvent(QPaintEvent* e);
  void leaveEvent(QEvent* e);

private:
  int* selected;
  int actionIndex;
  int max_col;

  QTableWidgetItem* hoverItem;
};
