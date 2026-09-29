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
#include "messaging.h"

//	A broadcast is to be sent
void lmcMessaging::sendBroadcast(MessageType type, XmlMessage* pMessage) {
	prepareBroadcast(type, pMessage);
}

//	A message is to be sent
void lmcMessaging::sendMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	QString data = QString();
	XmlMessage message;

	switch(type) {
	case MT_Group:
		data = pMessage->data(XN_GROUP);
		updateUser(type, *lpszUserId, data);
		break;
	case MT_Status:
	case MT_UserName:
	case MT_Note:
	case MT_PublicMessage:
		for(int index = 0; index < userList.count(); index++)
			prepareMessage(type, msgId, false, &userList[index].id, pMessage);
		msgId++;
		break;
	case MT_GroupMessage:
		if(lpszUserId)
			prepareMessage(type, msgId, false, lpszUserId, pMessage);
		else {
			for(int index = 0; index < userList.count(); index++)
				prepareMessage(type, msgId, false, &userList[index].id, pMessage);
		}
		msgId++;
		break;
	case MT_Avatar:
		//	if user id is specified send to that user alone, else send to all
        if(lpszUserId) {
			prepareMessage(type, msgId, false, lpszUserId, pMessage);
		} else {
            message = pMessage->clone();
            emit messageReceived(MT_Avatar, &localUser->id, &message);
			for(int index = 0; index < userList.count(); index++) {
				message = pMessage->clone();
				prepareMessage(type, msgId, false, &userList[index].id, &message);
			}
		}
		msgId++;
		break;
	case MT_Version:
		sendWebMessage(type, pMessage);
		break;
	default:
		prepareMessage(type, msgId, false, lpszUserId, pMessage);
		msgId++;
		break;
	}
}

void lmcMessaging::sendWebMessage(MessageType type, XmlMessage *pMessage) {
	Q_UNUSED(pMessage);

	QString szUrl;

	lmcTrace::write("Sending web message type " + QString::number(type));

	switch(type) {
	case MT_Version:
        szUrl = QString(IDA_DOMAIN "/version");
		pNetwork->sendWebMessage(&szUrl, NULL);
		break;
	default:
		break;
	}
}

//	A broadcast has been received
void lmcMessaging::receiveBroadcast(DatagramHeader* pHeader, QString* lpszData) {
	MessageHeader* pMsgHeader = NULL;
	XmlMessage* pMessage = NULL;
	if(!Message::getHeader(lpszData, &pMsgHeader, &pMessage)) {
		lmcTrace::write("Warning: Broadcast header parse failed");
		return;
	}
	pMsgHeader->address = pHeader->address;
	processBroadcast(pMsgHeader, pMessage);
}

//	A message has been received
void lmcMessaging::receiveMessage(DatagramHeader* pHeader, QString* lpszData) {
	MessageHeader* pMsgHeader = NULL;
	XmlMessage* pMessage = NULL;
	if(!Message::getHeader(lpszData, &pMsgHeader, &pMessage)) {
		lmcTrace::write("Warning: Message header parse failed");
		return;
	}
	pMsgHeader->address = pHeader->address;
	processMessage(pMsgHeader, pMessage);
}

//	A web message has been received
void lmcMessaging::receiveWebMessage(QString *lpszData) {
	MessageHeader* pMsgHeader = NULL;
	XmlMessage* pMessage = NULL;
	if(!Message::getHeader(lpszData, &pMsgHeader, &pMessage)) {
		lmcTrace::write("Warning: Web message header parse failed");
		return;
	}

	processWebMessage(pMsgHeader, pMessage);
}

//	Handshake procedure has been completed
void lmcMessaging::newConnection(QString* lpszUserId, QString* lpszAddress) {
	lmcTrace::write("Connection completed with user " + *lpszUserId + " at " + *lpszAddress);
	sendUserData(MT_UserData, QO_Get, lpszUserId, lpszAddress);
}

void lmcMessaging::connectionLost(QString* lpszUserId) {
	lmcTrace::write("Connection to user " + *lpszUserId + " lost");
	removeUser(*lpszUserId);
}

void lmcMessaging::sendUserData(MessageType type, QueryOp op, QString* lpszUserId, QString* lpszAddress) {
	lmcTrace::write("Sending local user details to user " + *lpszUserId + " at " + *lpszAddress);
	XmlMessage xmlMessage;
	xmlMessage.addData(XN_USERID, localUser->id);
	xmlMessage.addData(XN_NAME, localUser->name);
	xmlMessage.addData(XN_ADDRESS, localUser->address);
	xmlMessage.addData(XN_VERSION, localUser->version);
	xmlMessage.addData(XN_STATUS, localUser->status);
	xmlMessage.addData(XN_NOTE, localUser->note);
    xmlMessage.addData(XN_USERCAPS, QString::number(localUser->caps));
	xmlMessage.addData(XN_QUERYOP, QueryOpNames[op]);
	QString szMessage = Message::addHeader(type, msgId, &localUser->id, lpszUserId, &xmlMessage);
	pNetwork->sendMessage(lpszUserId, lpszAddress, &szMessage);
}

void lmcMessaging::prepareBroadcast(MessageType type, XmlMessage* pMessage) {
    if(!isConnected()) {
        lmcTrace::write("Warning: Not connected. Broadcast not sent");
        return;
    }
    if(localUser->id.isNull()) {
        lmcTrace::write("Warning: Local user not initialized. Broadcast not sent");
        return;
    }

	lmcTrace::write("Sending broadcast type " + QString::number(type));
	QString szMessage = Message::addHeader(type, msgId, &localUser->id, NULL, pMessage);
	pNetwork->sendBroadcast(&szMessage);
	lmcTrace::write("Broadcast sending done");
}

//	This method converts a Message from ui layer to a Datagram that can be passed to network layer
void lmcMessaging::prepareMessage(MessageType type, qint64 msgId, bool retry, QString* lpszUserId, XmlMessage* pMessage) {
	if(!isConnected()) {
		lmcTrace::write("Warning: Not connected. Message not sent");
		return;
	}
    if(localUser->id.isNull()) {
        lmcTrace::write("Warning: Local user not initialized. Message not sent");
        return;
    }

	User* receiver = getUser(lpszUserId);

	switch(type) {
	case MT_Status:
		pMessage->addData(XN_STATUS, localUser->status);
		break;
	case MT_UserName:
		pMessage->addData(XN_NAME, localUser->name);
		break;
	case MT_Note:
		pMessage->addData(XN_NOTE, localUser->note);
		break;
	case MT_Ping:
		//	add message to pending list
		if(!retry)
			addPendingMsg(msgId, MT_Ping, lpszUserId, pMessage);
		break;
	case MT_Message:
		if(!receiver) {
			emit messageReceived(MT_Failed, lpszUserId, pMessage);
			break;
		}
		//	add message to pending list
		if(!retry)
			addPendingMsg(msgId, MT_Message, lpszUserId, pMessage);
		break;
	case MT_GroupMessage:
		break;
	case MT_PublicMessage:
		break;
	case MT_Broadcast:
		break;
	case MT_Acknowledge:
		break;
	case MT_Query:
		//	if its a 'get' query add message to pending list
		if(pMessage->data(XN_QUERYOP) == QueryOpNames[QO_Get] && !retry)
			addPendingMsg(msgId, MT_Query, lpszUserId, pMessage);
		else if(pMessage->data(XN_QUERYOP) == QueryOpNames[QO_Result])
			getUserInfo(pMessage);
		break;
	case MT_ChatState:
		break;
	case MT_File:
	case MT_Avatar:
		prepareFile(type, msgId, retry, lpszUserId, pMessage);
		break;
    case MT_Folder:
        prepareFolder(type, msgId, retry, lpszUserId, pMessage);
        break;
    default:
        break;
	}

	if(!receiver) {
		lmcTrace::write("Warning: Recipient " + *lpszUserId + " not found. Message not sent");
		return;
	}

	lmcTrace::write("Sending message type " + QString::number(type) + " to user " + receiver->id
		+ " at " + receiver->address);
	QString szMessage = Message::addHeader(type, msgId, &localUser->id, lpszUserId, pMessage);
	pNetwork->sendMessage(&receiver->id, &receiver->address, &szMessage);
	lmcTrace::write("Message sending done");
}

//	This method converts a Datagram from network layer to a Message that can be passed to ui layer
void lmcMessaging::processBroadcast(MessageHeader* pHeader, XmlMessage* pMessage) {
	Q_UNUSED(pMessage);

	//	do not process broadcasts from local user unless loopback is specified in command line
	if(!loopback && pHeader->userId.compare(localUser->id) == 0)
		return;

	lmcTrace::write("Processing broadcast type " + QString::number(pHeader->type) + " from user " +
		pHeader->userId);

	switch(pHeader->type) {
	case MT_Announce:
		if(!getUser(&pHeader->userId))
			pNetwork->addConnection(&pHeader->userId, &pHeader->address);
		break;
	case MT_Depart:
		removeUser(pHeader->userId);
		break;
    default:
        break;
	}

	lmcTrace::write("Broadcast processing done");
}

void lmcMessaging::processMessage(MessageHeader* pHeader, XmlMessage* pMessage) {
	QString msgId;
	QString data = QString();
	XmlMessage reply;

	lmcTrace::write("Processing message type " + QString::number(pHeader->type) + " from user " +
		pHeader->userId);

	switch(pHeader->type) {
	case MT_UserData:
		if(pMessage->data(XN_QUERYOP) == QueryOpNames[QO_Get])
			sendUserData(pHeader->type, QO_Result, &pHeader->userId, &pHeader->address);
		//	add the user only after sending back user data, this way both parties will have added each other
		addUser(pMessage->data(XN_USERID), pMessage->data(XN_VERSION), pMessage->data(XN_ADDRESS),
            pMessage->data(XN_NAME), pMessage->data(XN_STATUS), QString(), pMessage->data(XN_NOTE),
            pMessage->data(XN_USERCAPS));
		break;
	case MT_Broadcast:
		emit messageReceived(pHeader->type, &pHeader->userId, pMessage);
		break;
	case MT_Status:
		data = pMessage->data(XN_STATUS);
		updateUser(pHeader->type, pHeader->userId, data);
		break;
	case MT_UserName:
		data = pMessage->data(XN_NAME);
		updateUser(pHeader->type, pHeader->userId, data);
		break;
	case MT_Note:
		data = pMessage->data(XN_NOTE);
		updateUser(pHeader->type, pHeader->userId, data);
		break;
	case MT_Message:
		//	add message to received message list
		if(addReceivedMsg(pHeader->id, pHeader->userId)) {
			emit messageReceived(pHeader->type, &pHeader->userId, pMessage);
		}
		//	send an acknowledgement
		msgId = QString::number(pHeader->id);
		reply.addData(XN_MESSAGEID, msgId);
		sendMessage(MT_Acknowledge, &pHeader->userId, &reply);
		break;
	case MT_GroupMessage:
		emit messageReceived(pHeader->type, &pHeader->userId, pMessage);
		break;
	case MT_PublicMessage:
		emit messageReceived(pHeader->type, &pHeader->userId, pMessage);
		break;
	case MT_Ping:
		//	send an acknowledgement
		msgId = QString::number(pHeader->id);
		reply.addData(XN_MESSAGEID, msgId);
		sendMessage(MT_Acknowledge, &pHeader->userId, &reply);
		break;
	case MT_Query:
		//	send a reply cum acknowledgement if its a 'get' query
		if(pMessage->data(XN_QUERYOP) == QueryOpNames[QO_Get]) {
			msgId = QString::number(pHeader->id);
			reply.addData(XN_MESSAGEID, msgId);
			reply.addData(XN_QUERYOP, QueryOpNames[QO_Result]);
			sendMessage(pHeader->type, &pHeader->userId, &reply);
		} else if(pMessage->data(XN_QUERYOP) == QueryOpNames[QO_Result]) {
			msgId = pMessage->data(XN_MESSAGEID);
			removePendingMsg(msgId.toLongLong());
            //  Add the path to the user's avatar image stored locally
            data = "avt_" + pHeader->userId + ".png";
            data = QDir(StdLocation::cacheDir()).absoluteFilePath(data);
            pMessage->addData(XN_AVATAR, data);
			emit messageReceived(pHeader->type, &pHeader->userId, pMessage);
		}
		break;
	case MT_ChatState:
		emit messageReceived(pHeader->type, &pHeader->userId, pMessage);
		break;
	case MT_Acknowledge:
		//	remove message from pending list
		msgId = pMessage->data(XN_MESSAGEID);
		removePendingMsg(msgId.toLongLong());
		break;
	case MT_File:
	case MT_Avatar:
		processFile(pHeader, pMessage);
		break;
    case MT_Folder:
        processFolder(pHeader, pMessage);
        break;
    default:
        break;
	}

	lmcTrace::write("Message processing done");
}

void lmcMessaging::processWebMessage(MessageHeader* pHeader, XmlMessage *pMessage) {
	lmcTrace::write("Processing web message type " + QString::number(pHeader->type));

	switch(pHeader->type) {
	case MT_Version:
		emit messageReceived(pHeader->type, NULL, pMessage);
		break;
	case MT_WebFailed:
		emit messageReceived(pHeader->type, NULL, pMessage);
		break;
	default:
		break;
	}
}
