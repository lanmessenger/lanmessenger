// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QVariant>
#include <QPointer>
#include "core/definitions.h"

class lmcSettings;
class lmcMessaging;
class lmcMainWindow;
class lmcChatWindow;
class lmcChatRoomWindow;
class lmcTransferWindow;
class lmcHistoryWindow;
class lmcSettingsDialog;
class lmcUserInfoWindow;
class lmcHelpWindow;
class lmcUpdateWindow;
class lmcUserSelectDialog;
class lmcAboutDialog;
class lmcBroadcastWindow;
class XmlMessage;
class QTimer;
class QRect;
class QWidget;

class lmcCore : public QObject {
  Q_OBJECT

public:
  lmcCore(void);
  ~lmcCore(void);
  void init(const QString& szCommandArgs);
  bool start(void);

public slots:
  bool processStartupSwitches(const QString& szMessage);
  void receiveRemoteMessage(const QString& message);
  void aboutToExit(void);

private slots:
  void exitApp(void);
  void timer_timeout(void);
  void startChat(QString* lpszUserId);
  void startChatRoom(QString* lpszThreadId);
  void sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void connectionStateChanged(void);
  void showTransfers(void);
  void showHistory(void);
  void showSettings(void);
  void showHelp(QRect* pRect);
  void showUpdate(QRect* pRect);
  void showAbout(void);
  void showBroadcast(void);
  void showPublicChat();
  void historyCleared(void);
  void fileHistoryCleared(void);
  void showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle = QString(), TrayMessageIcon icon = TMI_Info);
  void updateGroup(GroupOp op, QVariant value1, QVariant value2);
  void addContacts(QStringList* pExcludList);
  void chatWindow_closed(QString* lpszUserId);
  void chatRoomWindow_closed(QString* lpszThreadId);

private:
  void stop(void);
  void loadSettings(void);
  void settingsChanged(void);
  void processMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void processFile(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void routeMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void routeGroupMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void processPublicMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void createTransferWindow(void);
  void showTransferWindow(bool show = false);
  void initFileTransfer(MessageType type, FileMode mode, QString* lpszUserId, XmlMessage* pMessage);
  void showUserInfo(XmlMessage* pMessage);
  void createChatWindow(QString* lpszUserId);
  void showChatWindow(lmcChatWindow* chatWindow, bool show, bool alert = false);
  void createChatRoomWindow(QString* lpszThreadId);
  void showChatRoomWindow(lmcChatRoomWindow* chatRoomWindow, bool show, bool alert = false, bool add = false);
  void showPublicChatWindow(bool show, bool alert = false, bool open = false);
  QStringList showSelectContacts(QWidget* parent, uint caps, QStringList* excludeList);
  void showPortConflictMessage(void);

  lmcSettings* pSettings;
  QTimer* pTimer;
  lmcMessaging* pMessaging;
  lmcMainWindow* pMainWindow;
  QList<lmcChatWindow*> chatWindows;
  QList<lmcChatRoomWindow*> chatRoomWindows;
  lmcTransferWindow* pTransferWindow;
  QPointer<lmcHistoryWindow> pHistoryWindow;
  QPointer<lmcSettingsDialog> pSettingsDialog;
  QPointer<lmcUserInfoWindow> pUserInfoWindow;
  QPointer<lmcHelpWindow> pHelpWindow;
  lmcUpdateWindow* pUpdateWindow;
  QPointer<lmcChatRoomWindow> pPublicChatWindow;
  QPointer<lmcUserSelectDialog> pUserSelectDialog;
  QPointer<lmcAboutDialog> pAboutDialog;
  QPointer<lmcBroadcastWindow> pBroadcastWindow;
  bool messageTop;
  bool pubMessagePop;
  bool watchdogArmed = false;
  QString lang;
  bool adaptiveRefresh;
  int refreshTime;
  XmlMessage* pInitParams;
};
