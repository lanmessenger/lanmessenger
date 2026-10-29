// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <QString>
#include <QHash>
#include <QColor>
#include "ui_chatwindow.h"
#include "core/definitions.h"
#include "chatdefinitions.h"

struct User;
class XmlMessage;
class lmcSettings;
class ChatMessageView;
class lmcSoundPlayer;
class lmcToolButton;
class EmojiPickerAction;
class EmojiHighlighter;
class QMenu;
class QAction;
class QToolBar;
class QFont;
class QEvent;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;

class lmcChatWindow : public QWidget {
  Q_OBJECT

public:
  lmcChatWindow(QWidget* parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~lmcChatWindow(void);

  void init(User* pLocalUser, User* pRemoteUser, bool connected);
  void stop(void);
  void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void connectionStateChanged(bool connected);
  void settingsChanged(void);

  //  Loopback self-chat: log the echo under an alias so it renders as
  //  incoming instead of grouping with the sent row.
  void enableLoopbackEcho(void);
  static QString* loopbackEchoSender(bool loopbackMode, QString* lpszUserId,
                                     const QString& localId, QString* pEchoId);

  QString localId;
  QHash<QString, QString> peerIds;
  QString threadId;
  bool groupMode;

signals:
  void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void showHistory(void);
  void showTransfers(void);
  void closed(QString* lpszUserId);

protected:
  bool eventFilter(QObject* pObject, QEvent* pEvent);
  void changeEvent(QEvent* pEvent);
  void closeEvent(QCloseEvent* pEvent);
  void dragEnterEvent(QDragEnterEvent* pEvent);
  void dropEvent(QDropEvent* pEvent);

private slots:
  void btnFont_clicked(void);
  void btnFontColor_clicked(void);
  void btnFile_clicked(void);
  void btnFolder_clicked(void);
  void btnSave_clicked(void);
  void btnHistory_clicked(void);
  void btnTransfers_clicked(void);
  void btnSend_clicked(void);
  void emojiAction_triggered(void);
  void log_sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void checkChatState(void);

private:
  void createEmojiMenu(void);
  void createToolBar(void);
  void setUIText(void);
  void sendMessage(void);
  void sendFile(QString* lpszFilePath);
  void sendFolder(QString* lpszFolderPath);
  void sendObject(MessageType type, QString* lpszPath);
  void processFileOp(XmlMessage* pMessage);
  void appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage);
  void updateFileMessage(FileMode mode, FileOp op, QString fileId);
  void showStatus(int flag, bool add);
  QString getWindowTitle(void);
  void setMessageFont(QFont& font);
  void setChatState(ChatState newChatState);

  QString peerId;
  QString localName;
  QHash<QString, QString> peerNames;
  QHash<QString, QString> peerStatuses;
  QHash<QString, uint> peerCaps;
  User* pLocalUser;
  QString lastUserId;

  Ui::ChatWindow ui;
  lmcSettings* pSettings;
  ChatMessageView* pMessageLog;
  QAction* pFontAction;
  QAction* pFontColorAction;
  lmcToolButton* pbtnEmoji;
  QAction* pFileAction;
  QAction* pFolderAction;
  QAction* pSaveAction;
  QToolBar* pRightBar;
  QAction* pHistoryAction;
  QAction* pTransferAction;
  QMenu* pEmojiMenu;
  EmojiPickerAction* pEmojiAction;
  int nEmoji = -1;
  bool bConnected;
  int infoFlag;
  bool sendKeyMod;
  bool clearOnClose;
  bool showSendButton;
  lmcSoundPlayer* pSoundPlayer;
  QColor messageColor;
  bool messageColorSet = false;
  bool messageFontSet = false;
  //  Paints emoji runs in the color font. Created in init().
  EmojiHighlighter* emojiHighlighter = nullptr;
  ChatState chatState;
  qint64 keyStroke;
  qint64 snapKeyStroke;
  bool dataSaved;
  bool loopbackMode;
  QString loopbackEchoId;
};
