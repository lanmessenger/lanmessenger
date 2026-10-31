// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <QList>
#include "ui_historywindow.h"
#include "storage/historystore.h"

class lmcSettings;
class ChatMessageView;
class QTreeWidgetItem;
class QEvent;

class lmcHistoryWindow : public QWidget {
  Q_OBJECT

public:
  lmcHistoryWindow(QWidget* parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~lmcHistoryWindow();

  void init(void);
  void updateList(void);
  void stop(void);
  void settingsChanged(void);

protected:
  bool eventFilter(QObject* pObject, QEvent* pEvent);
  void changeEvent(QEvent* pEvent);

private slots:
  void tvMsgList_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
  void btnClearHistory_clicked(void);

private:
  void setUIText(void);
  void displayList(void);

  Ui::HistoryWindow ui;
  lmcSettings* pSettings;
  ChatMessageView* pMessageLog;
  QList<SessionInfo> msgList;
};
