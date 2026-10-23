// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <QString>
#include <QHash>
#include <QColor>
#include "ui_chatroomwindow.h"
#include "core/definitions.h"

struct User;
class XmlMessage;
class lmcSettings;
class ChatMessageView;
class lmcSoundPlayer;
class lmcToolButton;
class lmcImagePickerAction;
class QMenu;
class QAction;
class QTreeWidgetItem;
class QPoint;
class QFont;
class QEvent;
class QCloseEvent;

class lmcChatRoomWindow : public QWidget {
  Q_OBJECT

public:
  explicit lmcChatRoomWindow(QWidget* parent = 0);
  ~lmcChatRoomWindow();

  void init(User* pLocalUser, bool connected, QString thread = QString());
  void show(void);
  void stop(void);
  void addUser(User* pUser);
  void updateUser(User* pUser);
  void removeUser(QString* lpszUserId);
  void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void connectionStateChanged(bool connected);
  void settingsChanged(void);
  void selectContacts(QStringList* selectedContacts);

  QString threadId;
  QHash<QString, QString> peerIds;

signals:
  void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void chatStarting(QString* lpszUserId);
  void contactsAdding(QStringList* excludeList);
  void closed(QString* lpszThreadId);

protected:
  bool eventFilter(QObject* pObject, QEvent* pEvent);
  void changeEvent(QEvent* pEvent);
  void closeEvent(QCloseEvent* pEvent);

private slots:
  void userConversationAction_triggered(void);
  void userFileAction_triggered(void);
  void userInfoAction_triggered(void);
  void btnFont_clicked(void);
  void btnFontColor_clicked(void);
  void btnSave_clicked(void);
  void btnSend_clicked(void);
  void smileyAction_triggered(void);
  void addContactAction_triggered(void);
  void log_sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void tvUserList_itemActivated(QTreeWidgetItem* pItem, int column);
  void tvUserList_itemContextMenu(QTreeWidgetItem* pItem, QPoint& pos);

private:
  void createUserMenu(void);
  void createSmileyMenu(void);
  void createToolBar(void);
  void setUIText(void);
  void sendMessage(void);
  void encodeMessage(QString* lpszMessage);
  void appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage);
  void showStatus(int flag, bool add);
  QString getWindowTitle(void);
  void setMessageFont(QFont& font);
  void updateStatusImage(QTreeWidgetItem* pItem, QString* lpszStatus);
  QTreeWidgetItem* getUserItem(QString* lpszUserId);
  QTreeWidgetItem* getGroupItem(QString* lpszGroupId);
  void setUserAvatar(QString* lpszUserId, QString* lpszFilePath = 0);

  QString localId;
  QString localName;
  QHash<QString, QString> peerNames;
  User* pLocalUser;
  QString lastUserId;
  bool groupMode;

  Ui::ChatRoomWindow ui;
  lmcSettings* pSettings;
  ChatMessageView* pMessageLog;
  QMenu* pUserMenu;
  QAction* userChatAction;
  QAction* userFileAction;
  QAction* userInfoAction;
  QAction* pFontAction;
  QAction* pFontColorAction;
  lmcToolButton* pbtnSmiley;
  QAction* pSaveAction;
  QMenu* pSmileyMenu;
  lmcImagePickerAction* pSmileyAction;
  QAction* addContactAction;
  int nSmiley;
  bool bConnected;
  int infoFlag;
  bool showSmiley;
  bool sendKeyMod;
  bool showSendButton;
  lmcSoundPlayer* pSoundPlayer;
  QColor messageColor;
  bool messageColorSet = false;
  bool messageFontSet = false;
  bool dataSaved;
  bool windowLoaded;
};
