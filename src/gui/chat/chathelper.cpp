// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QFile>
#include <QDataStream>
#include "chathelper.h"

QDataStream& operator<<(QDataStream& out, const SingleMessage& message) {
  out << qint32(message.type) << message.userId << message.userName << message.message.toString()
      << message.id;
  return out;
}

QDataStream& operator>>(QDataStream& in, SingleMessage& message) {
  qint32 type;
  QString userId;
  QString userName;
  QString xmlMessage;
  QString id;
  in >> type >> userId >> userName >> xmlMessage >> id;
  message = SingleMessage((MessageType)type, userId, userName, XmlMessage(xmlMessage), id);
  return in;
}

void ChatHelper::makeHtmlSafe(QString* lpszMessage) {
  for(int index = 0; index < HTMLESC_COUNT; index++)
    lpszMessage->replace(htmlSymbol[index], htmlEscape[index]);
}

QString ChatHelper::replaceSmiley(QString* lpszHtml) {
  //<img src="qrc:/smileys/01">
  for(int index = 0; index < SM_MAPCOUNT; index++) {
    if(lpszHtml->compare("<img src=\"qrc" + smileyPic[index] + "\">") == 0) {
      QString code = smileyCode[index];
      makeHtmlSafe(&code);
      return code;
    }
  }

  return QString();
}

void ChatHelper::encodeSmileys(QString* lpszMessage) {
  //	replace all emoticon images with corresponding text code
  for(int index = 0; index < SM_MAPCOUNT; index++) {
    QString code = smileyCode[index];
    makeHtmlSafe(&code);
    lpszMessage->replace("<img src=\"" + smileyPic[index] + "\" />", code);
  }
}

void ChatHelper::decodeSmileys(QString* lpszMessage) {
  //	replace text emoticons with corresponding images
  for(int index = 0; index < SM_MAPCOUNT; index++) {
    QString code = smileyCode[index];
    makeHtmlSafe(&code);
    lpszMessage->replace(code, "<img src='qrc" + smileyPic[index] + "' />", Qt::CaseInsensitive);
  }
}

QTextBlockData::QTextBlockData(QString id) {
  this->id = id;
}

QTextBlockData::~QTextBlockData() {
}
