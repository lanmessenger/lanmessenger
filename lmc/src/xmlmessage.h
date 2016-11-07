/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
** 
** Contact:  qualiatech@gmail.com
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/


#ifndef XMLMESSAGE_H
#define XMLMESSAGE_H

#include <QDomDocument>
#include "definitions.h"

#define XN_ROOT				APP_MARKER
#define XN_HEAD				"head"
#define XN_BODY				"body"
#define XN_FROM				"from"
#define XN_TO				"to"
#define XN_MESSAGEID		"messageid"
#define XN_TYPE				"type"
#define XN_TIME				"time"
#define XN_KEY				"key"
#define XN_ADDRESS			"address"
#define XN_USERID			"userid"
#define XN_NAME				"name"
#define XN_VERSION			"version"
#define XN_PRESENCE			"presence"
#define XN_STATUS			"status"
#define XN_AVATAR			"avatar"
#define XN_LOGON			"logon"
#define XN_HOST				"host"
#define XN_OS				"os"
#define XN_FIRSTNAME		"firstname"
#define XN_LASTNAME			"lastname"
#define XN_ABOUT			"about"
#define XN_THREAD			"thread"
#define XN_MESSAGE			"message"
#define XN_GROUPMESSAGE		"groupmessage"
#define XN_BROADCAST		"broadcast"
#define XN_MODE				"mode"
#define XN_FILEOP			"fileop"
#define XN_FILETYPE			"filetype"
#define XN_FILEID			"fileid"
#define XN_FILEPATH			"filepath"
#define XN_FILENAME			"filename"
#define XN_FILESIZE			"filesize"
#define XN_CHATSTATE		"chatstate"
#define XN_QUERY			"query"
#define XN_QUERYOP			"queryop"
#define XN_GROUP			"group"
#define XN_FONT				"font"
#define XN_COLOR			"color"
#define XN_TEMPID			"tempid"
#define XN_ERROR			"error"
#define XN_GROUPMSGOP		"groupmsgop"
#define XN_DESCRIPTION		"description"
#define XN_NOTE				"note"
#define XN_SILENTMODE		"silentmode"
#define XN_TRACEMODE		"tracemode"
#define XN_LOGFILE			"logfile"
#define XN_PORT				"port"
#define XN_CONFIG			"config"
#define XN_USERCAPS         "usercaps"
#define XN_FOLDERID         "folderid"
#define XN_RELPATH          "relpath"
#define XN_FILECOUNT        "filecount"

class XmlMessage : public QDomDocument
{
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

#endif // XMLMESSAGE_H
