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


#ifndef SHARED_H
#define SHARED_H

#include <QString>
#include <QDateTime>
#include <QUuid>
#include <QHostInfo>
#include <QDataStream>
#include "definitions.h"
#ifdef QWIDGET_H
#include "uidefinitions.h"
#endif

struct User {
	QString id;
	QString name;
	QString address;
	QString version;
	QString status;
	int avatar;
	QString group;

	User(void) {}
	User(QString szId, QString szVersion, QString szAddress, QString szName, QString szStatus, QString szGroup, int nAvatar) {
		this->id = szId;
		this->version = szVersion;
		this->address = szAddress;
		this->name = szName;
		this->status = szStatus;
		this->group = szGroup;
		this->avatar = nAvatar;
	}
};

struct PendingMsg {
	qint64 msgId;
	bool active;
	QDateTime timeStamp;
	MessageType type;
	QString userId;
	QString data;
	int retry;

	PendingMsg(void) {}
	PendingMsg(qint64 nMsgId, bool bActive, QDateTime timeStamp, MessageType mtType, QString szUserId, QString szData, int nRetry) {
		this->msgId = nMsgId;
		this->active = bActive;
		this->timeStamp = timeStamp;
		this->type = mtType;
		this->userId = szUserId;
		this->data = szData;
		this->retry = nRetry;
	}
};

struct ReceivedMsg {
	qint64 msgId;
	QString userId;

	ReceivedMsg(void) {}
	ReceivedMsg(qint64 nMsgId, QString szUserId) {
		this->msgId = nMsgId;
		this->userId = szUserId;
	}

	bool operator == (const ReceivedMsg& v) const { return ((this->msgId == v.msgId) && (this->userId.compare(v.userId) == 0)); }
};

struct DatagramHeader {
	DatagramType type;
	QString userId;
	QString address;

	DatagramHeader(DatagramType dtType, QString szUserId, QString szAddress) {
		type = dtType;
		userId = szUserId;
		address = szAddress;
	}
};

struct MessageHeader {
	MessageType type;
	qint64 id;
	QString userId;
	QString address;

	MessageHeader(MessageType mtType, qint64 nId, QString szUserId) {
		type = mtType;
		id = nId;
		userId = szUserId;
	}
};

class Helper {
public:
	static int statusIndexFromCode(QString status);
	static QString formatSize(qint64 size);
	static QString getUuid(void);
	static QString getLogonName(void);
	static QString getHostName(void);
	static QString getOSName(void);
	static QString escapeDelimiter(QString *lpszData);
	static QString unescapeDelimiter(QString* lpszData);
};

#endif // SHARED_H