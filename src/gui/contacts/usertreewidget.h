// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef USERTREEWIDGET_H
#define USERTREEWIDGET_H

#include <QtWidgets/QApplication>
#include <QtWidgets/QTreeWidget>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QString>
#include <QtWidgets/QStyledItemDelegate>
#include <QPainter>
#include "core/uidefinitions.h"

class lmcUserTreeWidgetItem : public QTreeWidgetItem {
public:
  lmcUserTreeWidgetItem();
  ~lmcUserTreeWidgetItem() {}

  QRect checkBoxRect(const QRect& itemRect);
};

class lmcUserTreeWidgetGroupItem : public lmcUserTreeWidgetItem {
public:
  lmcUserTreeWidgetGroupItem()
      : lmcUserTreeWidgetItem() {}
  ~lmcUserTreeWidgetGroupItem() {}

  void addChild(QTreeWidgetItem* child);
};

class lmcUserTreeWidgetUserItem : public lmcUserTreeWidgetItem {
public:
  lmcUserTreeWidgetUserItem()
      : lmcUserTreeWidgetItem() {}
  ~lmcUserTreeWidgetUserItem() {}

private:
  bool operator<(const QTreeWidgetItem& other) const;
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
  void contextMenuEvent(QContextMenuEvent* event);
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
