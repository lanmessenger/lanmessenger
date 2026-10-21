// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "core/trace.h"
#include "message.h"

QString Message::addHeader(MessageType type, qint64 id, QString* lpszLocalId, QString* lpszPeerId, XmlMessage* pMessage) {
  if(!pMessage)
    pMessage = new XmlMessage();

  // remove time stamp from message
  pMessage->removeHeader(XN_TIME);

  pMessage->addHeader(XN_FROM, *lpszLocalId);
  if(lpszPeerId)
    pMessage->addHeader(XN_TO, *lpszPeerId);
  pMessage->addHeader(XN_MESSAGEID, QString::number(id));
  pMessage->addHeader(XN_TYPE, MessageTypeNames[type]);

  return pMessage->toString();
}

bool Message::getHeader(QString* lpszMessage, MessageHeader** ppHeader, XmlMessage** ppMessage) {
  *ppMessage = new XmlMessage(*lpszMessage);
  if(!((*ppMessage)->isValid()))
    return false;

  // add time stamp to message
  (*ppMessage)->addHeader(XN_TIME, QString::number(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));

  int type = Helper::indexOf(MessageTypeNames, MT_Max, (*ppMessage)->header(XN_TYPE));
  if(type < 0)
    return false;

  *ppHeader = new MessageHeader(
    (MessageType)type,
    (*ppMessage)->header(XN_MESSAGEID).toLongLong(),
    (*ppMessage)->header(XN_FROM));
  return true;
}
