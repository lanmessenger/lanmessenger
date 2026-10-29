// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include "ui_broadcastwindow.h"
#include "core/definitions.h"

class lmcSettings;
class XmlMessage;
class QToolBar;
class QToolButton;
class lmcToolButton;
class EmojiPickerAction;
class EmojiHighlighter;
class QMenu;
class QAction;
class QActionGroup;
class QTreeWidgetItem;
class QEvent;
class QCloseEvent;

class lmcBroadcastWindow : public QWidget {
  Q_OBJECT

public:
  lmcBroadcastWindow(QWidget* parent = 0);
  ~lmcBroadcastWindow();

  void init(bool connected);
  void stop(void);
  void show(QList<QTreeWidgetItem*>* pGroupList = 0);
  void connectionStateChanged(bool connected);
  void settingsChanged(void);

signals:
  void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);

protected:
  bool eventFilter(QObject* pObject, QEvent* pEvent);
  void changeEvent(QEvent* pEvent);
  void closeEvent(QCloseEvent* pEvent);

private slots:
  void btnFontSize_clicked(void);
  void fontAction_triggered(QAction* action);
  void emojiAction_triggered(void);
  void btnSelectAll_clicked(void);
  void btnSelectNone_clicked(void);
  void tvUserList_itemChanged(QTreeWidgetItem* item, int column);
  void btnSend_clicked(void);

private:
  void createToolBar(void);
  void setUIText(void);
  void sendMessage(void);
  void showStatus(int flag, bool add);

  Ui::BroadcastWindow ui;
  lmcSettings* pSettings;
  QToolBar* pToolBar;
  QToolButton* pbtnFontSize;
  lmcToolButton* pbtnEmoji;
  QMenu* pEmojiMenu;
  EmojiPickerAction* pEmojiAction;
  int fontSizeVal;
  int nEmoji = -1;
  //  Created in init(); broadcast has no setMessageFont.
  EmojiHighlighter* emojiHighlighter = nullptr;
  bool bConnected;
  int infoFlag;
  bool sendKeyMod;
  bool parentToggling;
  bool childToggling;
  QActionGroup* pFontGroup;
};
