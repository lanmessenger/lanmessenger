// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include <QVariant>
#include <QSystemTrayIcon>
#include "ui_mainwindow.h"
#include "core/definitions.h"

struct User;
struct Group;
class XmlMessage;
class lmcSettings;
class lmcSoundPlayer;
class QMenuBar;
class QMenu;
class QAction;
class QActionGroup;
class QToolButton;
class QTreeWidgetItem;
class QPoint;
class QRect;
class QEvent;
class QCloseEvent;

class lmcMainWindow : public QWidget {
  Q_OBJECT

public:
  lmcMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~lmcMainWindow(void);

  void init(User* pLocalUser, QList<Group>* pGroupList, bool connected);
  void start(void);
  void show(void);
  void restore(void);
  void minimize(void);
  void stop(void);
  void setShuttingDown(bool shuttingDown);
  void addUser(User* pUser);
  void updateUser(User* pUser);
  void removeUser(QString* lpszUserId);
  void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void connectionStateChanged(bool connected);
  void settingsChanged(bool init = false);
  void showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle = QString(), TrayMessageIcon icon = TMI_Info);
  QList<QTreeWidgetItem*> getContactsList(void);

signals:
  void appExiting(void);
  void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void chatStarting(QString* lpszUserId);
  void chatRoomStarting(QString* lpszThreadId);
  void showTransfers(void);
  void showHistory(void);
  void showSettings(void);
  void showHelp(QRect* pRect);
  void showUpdate(QRect* pRect);
  void showAbout(void);
  void showBroadcast(void);
  void showPublicChat(void);
  void groupUpdated(GroupOp op, QVariant value1, QVariant value2);

protected:
  bool eventFilter(QObject* pObject, QEvent* pEvent);
  void closeEvent(QCloseEvent* pEvent);
  void changeEvent(QEvent* pEvent);

private slots:
  void sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void trayShowAction_triggered(void);
  void trayHistoryAction_triggered(void);
  void trayFileAction_triggered(void);
  void traySettingsAction_triggered(void);
  void trayAboutAction_triggered(void);
  void trayExitAction_triggered(void);
  void statusAction_triggered(QAction* action);
  void avatarAction_triggered(void);
  void avatarBrowseAction_triggered(void);
  void helpAction_triggered(void);
  void homePageAction_triggered(void);
  void updateAction_triggered(void);
  void chatRoomAction_triggered(void);
  void publicChatAction_triggered(void);
  void refreshAction_triggered(void);
  void trayIcon_activated(QSystemTrayIcon::ActivationReason reason);
  void trayMessage_clicked(void);
  void tvUserList_itemActivated(QTreeWidgetItem* pItem, int column);
  void tvUserList_itemContextMenu(QTreeWidgetItem* pItem, QPoint& pos);
  void tvUserList_itemDragDropped(QTreeWidgetItem* pItem);
  void tvUserList_currentItemChanged(QTreeWidgetItem* pCurrent, QTreeWidgetItem* pPrevious);
  void groupAddAction_triggered(void);
  void groupRenameAction_triggered(void);
  void groupDeleteAction_triggered(void);
  void userConversationAction_triggered(void);
  void userBroadcastAction_triggered(void);
  void userFileAction_triggered(void);
  void userFolderAction_triggered(void);
  void userInfoAction_triggered(void);
  void txtNote_returnPressed(void);
  void txtNote_lostFocus(void);

private:
  void createMainMenu(void);
  void createTrayMenu(void);
  void createTrayIcon(void);
  void createStatusMenu(void);
  void createAvatarMenu(void);
  void createGroupMenu(void);
  void createUserMenu(void);
  void createToolBar(void);
  void setUIText(void);
  void showMinimizeMessage(void);
  void initGroups(QList<Group>* pGroupList);
  void updateStatusImage(QTreeWidgetItem* pItem, QString* lpszStatus);
  void setAvatar(QString fileName = QString());
  QTreeWidgetItem* getUserItem(QString* lpszUserId);
  QTreeWidgetItem* getGroupItem(QString* lpszGroupId);
  QTreeWidgetItem* getGroupItemByName(QString* lpszGroupName);
  void sendMessage(MessageType type, QString* lpszUserId, QString* lpszMessage);
  void sendAvatar(QString* lpszUserId);
  void setUserAvatar(QString* lpszUserId, QString* lpszFilePath);
  void processTrayIconTrigger(void);
  void setTrayTooltip(void);

  Ui::MainWindow ui;
  lmcSettings* pSettings;
  QMenuBar* pMainMenu;
  QSystemTrayIcon* pTrayIcon;
  QMenu* pFileMenu;
  QMenu* pToolsMenu;
  QMenu* pTrayMenu;
  QMenu* pHelpMenu;
  QMenu* pStatusMenu;
  QMenu* pAvatarMenu;
  QMenu* pGroupMenu;
  QMenu* pUserMenu;
  QToolButton* btnStatus;
  QAction* toolChatAction;
  QAction* toolFileAction;
  QAction* toolBroadcastAction;
  QAction* toolChatRoomAction;
  QAction* toolPublicChatAction;
  User* pLocalUser;
  bool bConnected;
  int nAvatar;
  bool showSysTray;
  bool showSysTrayMsg;
  bool showMinimizeMsg;
  bool minimizeHide;
  bool singleClickActivation;
  bool allowSysTrayMinimize;
  bool showAlert;
  bool noBusyAlert;
  bool noDNDAlert;
  bool statusToolTip;
  bool shuttingDown;
  lmcSoundPlayer* pSoundPlayer;
  TrayMessageType lastTrayMessageType;
  QActionGroup* statusGroup;
  QAction* chatRoomAction;
  QAction* publicChatAction;
  QAction* refreshAction;
  QAction* exitAction;
  QAction* historyAction;
  QAction* transferAction;
  QAction* settingsAction;
  QAction* helpAction;
  QAction* onlineAction;
  QAction* updateAction;
  QAction* aboutAction;
  QAction* trayShowAction;
  QAction* trayStatusAction;
  QAction* trayHistoryAction;
  QAction* trayTransferAction;
  QAction* traySettingsAction;
  QAction* trayAboutAction;
  QAction* trayExitAction;
  QAction* groupAddAction;
  QAction* groupRenameAction;
  QAction* groupDeleteAction;
  QAction* userChatAction;
  QAction* userBroadcastAction;
  QAction* userFileAction;
  QAction* userFolderAction;
  QAction* userInfoAction;
  QAction* avatarBrowseAction;
  bool windowLoaded;
};
