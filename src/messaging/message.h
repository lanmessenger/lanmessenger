// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QStringList>
#include "core/shared.h"
#include "xmlmessage.h"

enum MessagHeaderMember {
  MH_AppId = 0,
  MH_Type,
  MH_Id,
  MH_UserId,
  MH_Max
};

class Message {
public:
  static QString addHeader(MessageType type, qint64 id, QString* lpszLocalId, QString* lpszPeerId, XmlMessage* pMessage);
  static bool getHeader(QString* lpszMessage, MessageHeader** ppHeader, XmlMessage** ppMessage);
};

#endif // MESSAGE_H
