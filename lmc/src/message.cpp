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


#include "message.h"

QString Message::addHeader(MessageType type, qint64 id, QString* lpszUserId, QString* lpszData) {
	if(!lpszData)
		lpszData = new QString();

	QString message = escapeDelimiter(lpszData);

	message.insert(0, DELIMITER);
	message.insert(0, *lpszUserId);
	message.insert(0, DELIMITER);
	message.insert(0, QString::number(id));
	message.insert(0, DELIMITER);
	message.insert(0, QString::number((type)));
	message.insert(0, DELIMITER);
	message.insert(0, "LMC");

	return message;
}

bool Message::getHeader(QString* lpszMessage, MessageHeader** ppHeader) {
	QStringList headerList =  lpszMessage->split(DELIMITER, QString::SkipEmptyParts);
	if(headerList[MH_AppId].compare("LMC") != 0)
		return false;

	*ppHeader = new MessageHeader(
					(MessageType)headerList[MH_Type].toInt(),
					headerList[MH_Id].toLongLong(),
					headerList[MH_UserId]);
	return true;
}

QString Message::getData(QString* lpszDatagram) {
	QStringList list = lpszDatagram->split(DELIMITER, QString::SkipEmptyParts);
	if(list.count() > MH_Max)
		return unescapeDelimiter(&list[MH_Max]);

	return QString::null;
}

QString Message::escapeDelimiter(QString *lpszData) {
	return lpszData->replace(DELIMITER, DELIMITER_ESC, Qt::CaseSensitive);
}

QString Message::unescapeDelimiter(QString* lpszData) {
	return lpszData->replace(DELIMITER_ESC, DELIMITER, Qt::CaseSensitive);
}