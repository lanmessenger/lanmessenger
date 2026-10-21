// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <QList>
#include <QTreeWidget>
#include <qevent.h>
#include "ui_historywindow.h"
#include "core/settings.h"
#include "messaging/history.h"
#include "historytreewidget.h"
#include "gui/chat/messagelog.h"

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
  lmcMessageLog* pMessageLog;
  QList<MsgInfo> msgList;
};
