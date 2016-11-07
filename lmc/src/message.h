/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
** 
** Contact:  dilipvradhakrishnan@gmail.com
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


#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QStringList>
#include "shared.h"

enum MessagHeaderMember {
	MH_AppId = 0,
	MH_Type,
	MH_Id,
	MH_UserId,
	MH_Max
};

class Message {
public:
	static QString addHeader(MessageType type, qint64 id, QString* lpszUserId, QString* lpszData);
	static bool getHeader(QString* lpszMessage, MessageHeader** ppHeader);
	static QString getData(QString* lpszMessage);

private:
	static QString escapeDelimiter(QString *lpszData);
	static QString unescapeDelimiter(QString* lpszData);
};

#endif // MESSAGE_H