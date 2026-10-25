// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QTextBlockUserData>
#include "core/uidefinitions.h"
#include "chatdefinitions.h"
#include "messaging/xmlmessage.h"

class QTextBlockData : public QTextBlockUserData {
public:
  QTextBlockData(QString id);
  virtual ~QTextBlockData();

  QString id;
};

struct SingleMessage {
  MessageType type;
  QString userId;
  QString userName;
  XmlMessage message;
  QString id; // secondary id for more efficient traversal
  //  Lazy-once text signature (stored text is immutable); transient, not serialized.
  //  Covers body text (XN_MESSAGE/GROUPMESSAGE/BROADCAST) or chat-state only.
  //  File name/size are immutable after append and intentionally excluded:
  //  file rows share qHash("") and are told apart by FileOpRole in contentRev.
  mutable size_t textSigHash = 0;
  mutable int textSigLen = 0;
  mutable bool textSigOk = false;

  SingleMessage() {}
  SingleMessage(MessageType mType, QString szUserId, QString szUserName, XmlMessage xmlMessage, QString szId = QString()) {
    type = mType;
    userId = szUserId;
    userName = szUserName;
    message = xmlMessage;
    id = szId;
  }
};

QDataStream& operator<<(QDataStream& out, const SingleMessage& message);
QDataStream& operator>>(QDataStream& in, SingleMessage& message);

class ChatHelper {
public:
  static void makeHtmlSafe(QString* lpszMessage);
  static QString replaceSmiley(QString* lpszHtml);
  static void encodeSmileys(QString* lpszMessage);
  static void decodeSmileys(QString* lpszMessage);
};
