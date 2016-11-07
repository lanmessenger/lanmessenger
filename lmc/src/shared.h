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


#ifndef SHARED_H
#define SHARED_H

#include <QString>
#include <QDateTime>
#include <QUuid>
#include <QHostInfo>
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
	QString note;
    QString avatarPath;
    uint caps;

	User(void) {}
	User(QString szId, QString szVersion, QString szAddress, QString szName, QString szStatus, QString szGroup,
         int nAvatar, QString szNote, QString szAvatarPath, QString szCaps) {
		this->id = szId;
		this->version = szVersion;
		this->address = szAddress;
		this->name = szName;
		this->status = szStatus;
		this->group = szGroup;
		this->avatar = nAvatar;
		this->note = szNote;
        this->avatarPath = szAvatarPath;
        this->caps = szCaps.toUInt();
	}
};

struct Group {
	QString id;
	QString name;

	Group(void) {}
	Group(QString szId) {
		this->id = szId;
	}
	Group(QString szId, QString szName) {
		this->id = szId;
		this->name = szName;
	}

	bool operator == (const Group& v) const { return (this->id.compare(v.id) == 0); }
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
	static int indexOf(const QString array[], int size, const QString& value);
	static int statusIndexFromCode(QString status);
	static QString formatSize(qint64 size);
	static QString getUuid(void);
	static QString getLogonName(void);
	static QString getHostName(void);
	static QString getOSName(void);
	static QString escapeDelimiter(QString* lpszData);
	static QString unescapeDelimiter(QString* lpszData);
	static int compareVersions(const QString& version1, const QString& version2);
	static QString boolToString(bool value);
	static bool stringToBool(const QString& value);
	static bool copyFile(const QString& source, const QString& destination);
};

#endif // SHARED_H
