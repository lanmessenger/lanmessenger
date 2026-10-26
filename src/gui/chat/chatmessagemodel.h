// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QFont>
#include <QHash>
#include <QList>
#include <QMap>

#include "core/definitions.h"
#include "gui/chat/chathelper.h"
#include "messaging/xmlmessage.h"

//  Log save format (Html/legacy-compatible, plain text).
enum OutputFormat {
  HtmlFormat,
  TextFormat
};

//  Native chat log: a projection of SingleMessage/sendFileMap/receiveFileMap
//  for QListView + BubbleDelegate. Network, XmlMessage and QDataStream
//  save/restore are untouched. Grouping rule: same sender as previous row.
class ChatMessageModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum Roles {
    TypeRole = Qt::UserRole + 1,
    UserIdRole,
    UserNameRole,
    TimeRole,
    TextRole,
    FontRole,
    ColorRole,
    AvatarRole,
    GroupedRole,
    FileRole,
    OwnRole,
    PreviewRole,
    IdRole,
    FileOpRole,
    FileModeRole,
    FileNameRole,
    FileSizeRole
  };

  explicit ChatMessageModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  //  Mirrors lmcMessageLog::appendMessageLog (no HTML). Returns row or -1
  //  (ChatState is an ephemeral extra row, never stored; Error is dropped).
  int appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName,
                       XmlMessage* pMessage, bool bReload = false);
  void updateFileMessage(FileMode mode, FileOp op, QString fileId);
  void updateUserName(QString* lpszUserId, QString* lpszUserName);
  void updateAvatar(QString* lpszUserId, QString* lpszFilePath);
  void abortPendingFileOperations();
  void clearLog();

  //  File interactivity: updates FO_* and hands the XmlMessage out
  //  (view/window decides on emit messageSent).
  bool requestFileAction(const QString& fileId, const QString& action, const QString& fileType,
                         FileMode mode, XmlMessage* outMessage);

  const SingleMessage& messageAt(int row) const;
  //  Lazy-once text signature, O(1) after the first call (stored text is immutable).
  //  Covers body text / chat-state only; file name/size are immutable after
  //  append and excluded by design (see SingleMessage::textSigHash).
  size_t rowTextHash(int row, int* pLen = nullptr) const;
  //  Ephemeral typing row: always last when present, never in messages().
  bool isTypingRow(int row) const;
  const SingleMessage& typingMessage() const;
  QList<SingleMessage> messages() const;
  void setMessages(const QList<SingleMessage>& messages);
  QString plainText() const;
  QString messageText(const SingleMessage& msg) const;

  QString localId;
  QHash<QString, QString> participantAvatars;
  bool hasData = false;

private:
  QString fileTempId(FileMode mode, const QString& fileId) const;
  QString fileTempId(XmlMessage* pMessage) const;
  QString statusText(const SingleMessage& msg) const;
  void clearTypingRow();

  QList<SingleMessage> messageLog;
  //  Shown while the peer composes, dropped by any later append.
  bool hasTyping = false;
  SingleMessage typingMsg;
  QMap<QString, XmlMessage> sendFileMap;
  QMap<QString, XmlMessage> receiveFileMap;
};
