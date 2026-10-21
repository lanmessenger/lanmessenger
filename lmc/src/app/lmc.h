// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef LMC_H
#define LMC_H

#include <QObject>
#include <QTimer>
#include <QSysInfo>
#include <QPointer>
#include "core/shared.h"
#include "core/settings.h"
#include "messaging/messaging.h"
#include "gui/main/mainwindow.h"
#include "gui/chat/chatwindow.h"
#include "messaging/history.h"
#include "core/stdlocation.h"
#include "gui/files/transferwindow.h"
#include "gui/history/historywindow.h"
#include "gui/main/settingsdialog.h"
#include "gui/contacts/userinfowindow.h"
#include "gui/main/helpwindow.h"
#include "gui/main/updatewindow.h"
#include "gui/chat/chatroomwindow.h"
#include "gui/contacts/userselectdialog.h"
#include "gui/main/aboutdialog.h"
#include "gui/chat/broadcastwindow.h"

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
  QString lang;
  bool adaptiveRefresh;
  int refreshTime;
  XmlMessage* pInitParams;
};

#endif // LMC_H
