// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QTreeWidget>
#include "core/uidefinitions.h"

class lmcHistoryTreeWidgetItem : public QTreeWidgetItem {
public:
  lmcHistoryTreeWidgetItem(QTreeWidget* parent = 0)
      : QTreeWidgetItem(parent) {}
  ~lmcHistoryTreeWidgetItem() {}

  bool operator<(const QTreeWidgetItem& other) const {
    int column = treeWidget()->sortColumn();
    if(column == 1)
      return data(column, IdRole).toDateTime() < other.data(column, DataRole).toDateTime();
    else
      return text(column).toLower() < other.text(column).toLower();
  }
};
