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


#ifndef HISTORY_H
#define HISTORY_H

#include <QString>
#include <QFile>
#include <QDataStream>
#include <QDateTime>
#include <QList>
#include "settings.h"

#define HC_FILENAME		"messenger.db"
#define HC_HDRSIZE		28
#define HC_VERSION		1
#define HC_DBMARKER		"DB"
#define HC_IDMARKER		"ID"
#define HC_DTMARKER		"DT"

struct DBHeader {
	QString marker;
	int version;
	qint16 headerSize;
	int count;
	qint64 first;
	qint64 last;

	DBHeader() {
		this->marker = HC_DBMARKER;
		this->headerSize = HC_HDRSIZE;
		this->version = HC_VERSION;
	}

	DBHeader(QString szMarker, qint16 nHeaderSize, int nVersion, int nCount, qint64 nFirst, qint64 nLast) {
		this->marker = szMarker;
		this->headerSize = nHeaderSize;
		this->version = nVersion;
		this->count = nCount;
		this->first = nFirst;
		this->last = nLast;
	}
};

struct MsgInfo {
	QString name;
	QDateTime date;
	qint64 offset;

	MsgInfo() { }

	MsgInfo(QString name, QDateTime date, qint64 offset) {
		this->name = name;
		this->date = date;
		this->offset = offset;
	}
};

class History {
public:
	static QString historyFile(void);
	static int save(QString user, QDateTime timeStamp, QString* lpszData);
	static QList<MsgInfo> getList(void);
	static QString getMessage(qint64 offset);

private:
	static void create(QString path);
	static void writeHeader(QDataStream* pStream, DBHeader* pHeader);
	static DBHeader readHeader(QDataStream* pStream);
	static qint64 insertData(QDataStream* pStream, QString* lpszData);
	static qint64 insertIndex(QDataStream* pStream, qint64 dataPos, QString user, QDateTime timeStamp);
	static void updateIndex(QDataStream* pData, qint64 oldIndex, qint64 newIndex);
};

#endif // HISTORY_H