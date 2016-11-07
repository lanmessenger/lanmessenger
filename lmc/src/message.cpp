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


#include "trace.h"
#include "message.h"

QString Message::addHeader(MessageType type, qint64 id, QString* lpszLocalId, QString* lpszPeerId, XmlMessage* pMessage) {
	if(!pMessage)
		pMessage = new XmlMessage();

	// remove time stamp from message
	pMessage->removeHeader(XN_TIME);

	pMessage->addHeader(XN_FROM, *lpszLocalId);
	if(lpszPeerId)
		pMessage->addHeader(XN_TO, *lpszPeerId);
	pMessage->addHeader(XN_MESSAGEID, QString::number(id));
	pMessage->addHeader(XN_TYPE, MessageTypeNames[type]);

	return pMessage->toString();
}

bool Message::getHeader(QString* lpszMessage, MessageHeader** ppHeader, XmlMessage** ppMessage) {
	*ppMessage = new XmlMessage(*lpszMessage);
	if(!((*ppMessage)->isValid()))
		return false;

	// add time stamp to message
	(*ppMessage)->addHeader(XN_TIME, QString::number(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));

	int type = Helper::indexOf(MessageTypeNames, MT_Max, (*ppMessage)->header(XN_TYPE));
	if(type < 0)
		return false;

	*ppHeader = new MessageHeader(
					(MessageType)type,
					(*ppMessage)->header(XN_MESSAGEID).toLongLong(),
					(*ppMessage)->header(XN_FROM));
	return true;
}
