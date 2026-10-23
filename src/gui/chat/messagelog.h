// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QString>
#include <QHash>
#include <QMap>
#include <QList>
#include "core/definitions.h"
#include "gui/chat/chatdefinitions.h"
#include "gui/chat/chathelper.h"
#include "gui/chat/chatmessagemodel.h"
#include "messaging/xmlmessage.h"
#include "core/theme.h"
#include "gui/widgets/qmessagebrowser.h"

class QMenu;
class QAction;
class QEvent;
class QResizeEvent;
class QTextCursor;
class QFont;
class QColor;
class QDateTime;
class QUrl;
class QPoint;

class lmcMessageLog : public QMessageBrowser {
  Q_OBJECT

public:
  lmcMessageLog(QWidget* parent = nullptr);
  ~lmcMessageLog(void) override;

  void initMessageLog(QString themePath, bool clearLog = true);
  void reloadTheme();
  void appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage,
                        bool bReload = false);
  void updateFileMessage(FileMode mode, FileOp op, QString fileId);
  void updateUserName(QString* lpszUserId, QString* lpszUserName);
  void updateAvatar(QString* lpszUserId, QString* lpszFilePath);
  void reloadMessageLog(void);
  QString prepareMessageLogForSave(OutputFormat format = HtmlFormat);
  void setAutoScroll(bool enable);
  void abortPendingFileOperations(void);
  void saveMessageLog(QString filePath);
  void restoreMessageLog(QString filePath, bool reload = true);

  QString localId;
  QString peerId;
  QString peerName;
  QHash<QString, QString> participantAvatars;
  QString lastId;
  bool hasData;
  int fontSizeVal;
  bool showSmiley;
  bool autoFile;
  bool messageTime;
  bool messageDate;
  QString themePath;
  bool allowLinks;
  bool pathToLink;
  bool trimMessage;

signals:
  void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);

protected:
  virtual void changeEvent(QEvent* event) override;
  virtual void resizeEvent(QResizeEvent* event) override;

  void scrollToEnd(QTextCursor& cursor);

private slots:
  // TODO
  //void log_linkClicked(QUrl url);
  void log_linkHovered(const QString& link, const QString& title, const QString& textContent);
  void showContextMenu(const QPoint& pos);
  void copyAction_triggered(void);
  void copyLinkAction_triggered(void);
  void selectAllAction_triggered(void);
  void onAnchorClicked(const QUrl& arg1);

private:
  void createContextMenu(void);
  void appendMessageLog(QString* lpszHtml, MessageType type, QTextBlockData* data = nullptr);
  void removeMessageLog(MessageType type);
  void replaceMessageLog(MessageType type, QString id, QString html);
  void insertMessageLog(QTextCursor cursor, QString& html, MessageType type, QTextBlockData* data);
  bool isSameBlock(QTextCursor& cursor, MessageType type, QString& id) const;
  void appendBroadcast(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QDateTime* pTime);
  void appendMessage(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QDateTime* pTime,
                     QFont* pFont, QColor* pColor);
  void appendPublicMessage(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QDateTime* pTime,
                           QFont* pFont, QColor* pColor, MessageType messageType);
  QString getFileMessageText(MessageType type, QString* lpszUserName, XmlMessage* pMessage, bool bReload = false);
  QString getFontStyle(QFont* pFont, QColor* pColor, bool size = false);
  QString getFileStatusMessage(FileMode mode, FileOp op);
  QString getChatStateMessage(ChatState chatState);
  QString getChatRoomMessage(GroupMsgOp op);
  void fileOperation(QString fileId, QString action, QString fileType, FileMode mode = FM_Receive);
  void decodeMessage(QString* lpszMessage, bool useDefaults = false);
  void processMessageText(QString* lpszMessageText, bool useDefaults);
  QString getTimeString(QDateTime* pTime);
  void setUIText(void);
  QString getFileTempId(FileMode mode, QString fileId) const;
  QString getFileTempId(XmlMessage* pMessage) const;

  QMap<QString, XmlMessage> sendFileMap;
  QMap<QString, XmlMessage> receiveFileMap;
  QList<SingleMessage> messageLog;
  ThemeData themeData;
  QMenu* contextMenu;
  QAction* copyAction;
  QAction* copyLinkAction;
  QAction* selectAllAction;
  bool linkHovered;
  bool outStyle;
  bool autoScroll;
};
