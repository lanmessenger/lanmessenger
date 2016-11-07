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


#ifndef CHATHELPER_H
#define CHATHELPER_H

#include "uidefinitions.h"
#include "chatdefinitions.h"
#include "xmlmessage.h"

struct SingleMessage {
	MessageType type;
	QString userId;
	QString userName;
	XmlMessage message;
	QString id;	// secondary id for more efficient traversal

	SingleMessage() {}
	SingleMessage(MessageType mType, QString szUserId, QString szUserName, XmlMessage xmlMessage, QString szId = QString()) {
		type = mType;
		userId = szUserId;
		userName = szUserName;
		message = xmlMessage;
		id = szId;
	}
};

QDataStream &operator << (QDataStream &out, const SingleMessage &message);
QDataStream &operator >> (QDataStream &in, SingleMessage &message);

class ChatHelper {
public:
	static void makeHtmlSafe(QString* lpszMessage);
	static QString replaceSmiley(QString* lpszHtml);
	static void encodeSmileys(QString* lpszMessage);
	static void decodeSmileys(QString* lpszMessage);
};

#endif // CHATHELPER_H
