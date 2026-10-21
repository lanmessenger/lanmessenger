// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QDomDocument>
#include "core/definitions.h"

#define XN_ROOT APP_MARKER
#define XN_HEAD "head"
#define XN_BODY "body"
#define XN_FROM "from"
#define XN_TO "to"
#define XN_MESSAGEID "messageid"
#define XN_TYPE "type"
#define XN_TIME "time"
#define XN_KEY "key"
#define XN_ADDRESS "address"
#define XN_USERID "userid"
#define XN_NAME "name"
#define XN_VERSION "version"
#define XN_WINVERSION "winVersion"
#define XN_MACVERSION "macVersion"
#define XN_LINVERSION "linVersion"
#define XN_PRESENCE "presence"
#define XN_STATUS "status"
#define XN_AVATAR "avatar"
#define XN_LOGON "logon"
#define XN_HOST "host"
#define XN_OS "os"
#define XN_FIRSTNAME "firstname"
#define XN_LASTNAME "lastname"
#define XN_ABOUT "about"
#define XN_THREAD "thread"
#define XN_MESSAGE "message"
#define XN_GROUPMESSAGE "groupmessage"
#define XN_BROADCAST "broadcast"
#define XN_MODE "mode"
#define XN_FILEOP "fileop"
#define XN_FILETYPE "filetype"
#define XN_FILEID "fileid"
#define XN_FILEPATH "filepath"
#define XN_FILENAME "filename"
#define XN_FILESIZE "filesize"
#define XN_CHATSTATE "chatstate"
#define XN_QUERY "query"
#define XN_QUERYOP "queryop"
#define XN_GROUP "group"
#define XN_FONT "font"
#define XN_COLOR "color"
#define XN_TEMPID "tempid"
#define XN_ERROR "error"
#define XN_GROUPMSGOP "groupmsgop"
#define XN_DESCRIPTION "description"
#define XN_NOTE "note"
#define XN_ARGSILENTMODE "argsilentmode"
#define XN_ARGUDPPORT "argudpport"
#define XN_TCPPORT "tcpport"
#define XN_ARGCONFIG "argconfig"
#define XN_USERCAPS "usercaps"
#define XN_FOLDERID "folderid"
#define XN_RELPATH "relpath"
#define XN_FILECOUNT "filecount"

class XmlMessage : public QDomDocument {
public:
  XmlMessage(void);
  XmlMessage(const QString& text);
  ~XmlMessage(void);

  bool addHeader(const QString& nodeName, const QString& nodeValue);
  bool addData(const QString& nodeName, const QString& nodeValue);
  QString header(const QString& nodeName);
  QString data(const QString& nodeName);
  bool removeHeader(const QString& nodeName);
  bool removeData(const QString& nodeName);
  bool headerExists(const QString& nodeName);
  bool dataExists(const QString& nodeName);
  XmlMessage clone(void);
  bool isValid(void);

private:
  bool addXmlNode(const QString& parentNode, const QString& nodeName, const QString& nodeValue);
  QString getXmlNode(const QString& parentNode, const QString& nodeName);
  bool removeXmlNode(const QString& parentNode, const QString& nodeName);
  bool xmlNodeExists(const QString& parentNode, const QString& nodeName);
};
