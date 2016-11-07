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


#include <QDataStream>
#include "datagram.h"

QByteArray Datagram::addHeader(DatagramType type, QString* lpszUserId, QByteArray& baData) {
	QString szAppId("LMC");
	QByteArray userId = lpszUserId->toUtf8();
	quint32 headerLen = sizeof(quint32) + sizeof(quint32) + szAppId.length() + sizeof(qint16)
		+ sizeof(quint32) + lpszUserId->length();
	quint32 datagramLen = headerLen + sizeof(quint32) + baData.length();
	QByteArray datagram(datagramLen, '\0');
	QDataStream stream(&datagram, QIODevice::WriteOnly);
	
	stream << headerLen << szAppId << (qint16)type << *lpszUserId << (quint32)baData.length();
	stream.writeRawData(baData.data(), baData.length());

	return datagram;
}

bool Datagram::getHeader(QByteArray& baDatagram, DatagramHeader** ppHeader) {
	quint32 headerLen;
	QString szAppId;
	qint16 type;
	QString szUserId;

	QDataStream stream(baDatagram);
	stream >> headerLen >> szAppId >> type >> szUserId;

	if(szAppId.compare("LMC") != 0)
		return false;

	*ppHeader = new DatagramHeader(
					(DatagramType)type,
					szUserId,
					QString());
	return true;
}

QByteArray Datagram::getData(QByteArray& baDatagram) {
	quint32 headerLen;
	QString szAppId;
	qint16 type;
	QString szUserId;
	quint32 dataLen;

	QDataStream stream(baDatagram);
	stream >> headerLen >> szAppId >> type >> szUserId >> dataLen;

	if(dataLen > 0) {
		char* buffer = new char[dataLen];
		stream.readRawData(buffer, dataLen);
		return QByteArray(buffer, dataLen);
	}

	return QByteArray();
}
