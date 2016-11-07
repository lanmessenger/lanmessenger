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


#include "messaging.h"

//	A broadcast is to be sent
void lmcMessaging::sendBroadcast(MessageType type, QString* lpszData) {
	prepareBroadcast(type, lpszData);
}

//	A message is to be sent
void lmcMessaging::sendMessage(MessageType type, QString* lpszUserId, QString* lpszData) {
	QString userData = QString::null;

	switch(type) {
	case MT_Group:
		userData.append(QString::number(type) + DELIMITER + *lpszData);
		updateUser(*lpszUserId, userData);
		break;
	case MT_Status:
	case MT_Avatar:
	case MT_UserName:
		for(int index = 0; index < userList.count(); index++)
			prepareMessage(type, msgId, false, &userList[index].id, lpszData);
		msgId++;
		break;
	default:
		prepareMessage(type, msgId, false, lpszUserId, lpszData);
		msgId++;
		break;
	}
}

//	A broadcast has been received
void lmcMessaging::receiveBroadcast(DatagramHeader* pHeader, QString* lpszData) {
	MessageHeader* pMsgHeader = NULL;
	if(!Message::getHeader(lpszData, &pMsgHeader))
		return;
	pMsgHeader->address = pHeader->address;
	QString szData = Message::getData(lpszData);
	processBroadcast(pMsgHeader, &szData);
}

//	A message has been received
void lmcMessaging::receiveMessage(DatagramHeader* pHeader, QString* lpszData) {
	MessageHeader* pMsgHeader = NULL;
	if(!Message::getHeader(lpszData, &pMsgHeader))
		return;
	pMsgHeader->address = pHeader->address;
	QString szData = Message::getData(lpszData);
	processMessage(pMsgHeader, &szData);
}

//	Handshake procedure has been completed
void lmcMessaging::newConnection(QString* lpszUserId, QString* lpszAddress) {
	sendUserData(MT_UserData, lpszUserId, lpszAddress);
}

void lmcMessaging::connectionLost(QString* lpszUserId) {
	removeUser(*lpszUserId);
}

void lmcMessaging::receiveProgress(QString* lpszUserId, QString* lpszData) {
	QString sendData;
	QStringList fileData = lpszData->split(DELIMITER, QString::SkipEmptyParts);
	int fileOp = fileData[0].toInt();

	switch(fileOp) {
	case FO_ErrorSend:
		sendData.append(QString::number(FO_AbortSend) + DELIMITER + fileData[FD_Id]);
		sendMessage(MT_FileOp, lpszUserId, &sendData);
		break;
	case FO_ErrorReceive:
		sendData.append(QString::number(FO_AbortReceive) + DELIMITER + fileData[FD_Id]);
		sendMessage(MT_FileOp, lpszUserId, &sendData);
		break;
	}
	emit messageReceived(MT_FileOp, lpszUserId, lpszData);
}

void lmcMessaging::sendUserData(MessageType type, QString* lpszUserId, QString* lpszAddress) {
	QString data(localUser->id + DELIMITER + localUser->name + DELIMITER + localUser->address +
			DELIMITER + localUser->version + DELIMITER + localUser->status + DELIMITER + 
			QString::number(localUser->avatar));
	QString message = Message::addHeader(type, msgId, &localUser->id, &data);
	pNetwork->sendMessage(lpszUserId, lpszAddress, &message);
}

void lmcMessaging::prepareBroadcast(MessageType type, QString* lpszData) {
	QString message = Message::addHeader(type, msgId, &localUser->id, lpszData);
	pNetwork->sendBroadcast(&message);
}

//	This method converts a Message from ui layer to a Datagram that can be passed to network layer
void lmcMessaging::prepareMessage(MessageType type, qint64 msgId, bool retry, QString* lpszUserId, QString* lpszData) {
	if(!isConnected())
		return;

	User* receiver = getUser(lpszUserId);
	QString data;

	switch(type) {
	case MT_Status:
		data.append(QString::number(type) + DELIMITER + localUser->status);
		break;
	case MT_Avatar:
		data.append(QString::number(type) + DELIMITER + QString::number(localUser->avatar));
		break;
	case MT_UserName:
		data.append(QString::number(type) + DELIMITER + localUser->name);
		break;
	case MT_Ping:
		//	add message to pending list
		if(!retry)
			addPendingMsg(msgId, MT_Ping, *lpszUserId, data);
		break;
	case MT_Message:
		if(!receiver) {
			emit messageReceived(MT_Failed, lpszUserId, lpszData);
			break;
		}
		data.append(lpszData);
		//	add message to pending list
		if(!retry)
			addPendingMsg(msgId, MT_Message, *lpszUserId, *lpszData);
		break;
	case MT_Broadcast:
		data.append(lpszData);
		break;
	case MT_Acknowledge:
		data.append(lpszData);
		break;
	case MT_UserQuery:
		data.append(lpszData);
		//	add message to pending list
		if(!retry)
			addPendingMsg(msgId, MT_UserQuery, *lpszUserId, *lpszData);
		break;
	case MT_UserInfo:
		data.append(getUserInfo());
		break;
	case MT_UserAction:
		break;
	case MT_FileReq:
	case MT_FileOp:
		data.append(lpszData);
		prepareFile(type, msgId, retry, lpszUserId, lpszData);
		break;
    default:
        break;
	}

	if(!receiver)
		return;

	QString message = Message::addHeader(type, msgId, &localUser->id, &data);
	pNetwork->sendMessage(&receiver->id, &receiver->address, &message);
}

void lmcMessaging::prepareFile(MessageType type, qint64 msgId, bool retry, QString* lpszUserId, QString* lpszData) {
    Q_UNUSED(type);
    Q_UNUSED(msgId);
    Q_UNUSED(retry);

	QStringList fileData = lpszData->split(DELIMITER, QString::SkipEmptyParts);
	int fileOp = fileData[FD_Op].toInt();

	User* user = getUser(lpszUserId);

	switch(fileOp) {
	case FO_Request:
		pNetwork->initSendFile(&user->id, &user->address, lpszData);
		break;
	case FO_Accept:
		pNetwork->initReceiveFile(&user->id, &user->address, lpszData);
		break;
	case FO_CancelSend:
		pNetwork->fileOperation(FM_Send, &user->id, lpszData);
		break;
	case FO_CancelReceive:
		pNetwork->fileOperation(FM_Receive, &user->id, lpszData);
		break;
	}
}

//	This method converts a Datagram from network layer to a Message that can be passed to ui layer
void lmcMessaging::processBroadcast(MessageHeader* pHeader, QString* lpszData) {
    Q_UNUSED(lpszData);
	
	//	do not process broadcasts from local user unless loopback is specified in command line
	if(!loopback && pHeader->userId.compare(localUser->id) == 0)
		return;

	switch(pHeader->type) {
	case MT_Online:
		if(!getUser(&pHeader->userId))
			pNetwork->addConnection(&pHeader->userId, &pHeader->address);
		break;
	case MT_Offline:
		removeUser(pHeader->userId);
		break;
    default:
        break;
	}
}

void lmcMessaging::processMessage(MessageHeader* pHeader, QString* lpszData) {
	QString msgId;
	QString data = QString::null;
	QStringList userData;

	switch(pHeader->type) {
	case MT_UserData:
		userData = lpszData->split(DELIMITER, QString::SkipEmptyParts);
		addUser(userData[UD_Id], userData[UD_Version], userData[UD_Address], userData[UD_Name], 
			userData[UD_Status], userData[UD_Avatar]);
		sendUserData(MT_UserDataAck, &userData[UD_Id], &userData[UD_Address]);
		break;
	case MT_UserDataAck:
		userData = lpszData->split(DELIMITER, QString::SkipEmptyParts);
		addUser(userData[UD_Id], userData[UD_Version], userData[UD_Address], userData[UD_Name], 
			userData[UD_Status], userData[UD_Avatar]);
		break;
	case MT_Broadcast:
		emit messageReceived(MT_Broadcast, &pHeader->userId, lpszData);
		break;
	case MT_Status:
	case MT_Avatar:
	case MT_UserName:
		updateUser(pHeader->userId, *lpszData);
		break;
	case MT_Message:
		//	add message to received message list
		if(addReceivedMsg(pHeader->id, pHeader->userId)) {
			emit messageReceived(MT_Message, &pHeader->userId, lpszData);
		}
		//	send an acknowledgement
		msgId = QString::number(pHeader->id);
		sendMessage(MT_Acknowledge, &pHeader->userId, &msgId);
		break;
	case MT_Ping:
		//	send an acknowledgement
		msgId = QString::number(pHeader->id);
		sendMessage(MT_Acknowledge, &pHeader->userId, &msgId);
		break;
	case MT_UserQuery:
		//	send an acknowledgement
		msgId = QString::number(pHeader->id);
		sendMessage(MT_Acknowledge, &pHeader->userId, &msgId);
		//	send user information
		sendMessage(MT_UserInfo, &pHeader->userId, lpszData);
		break;
	case MT_UserInfo:
		emit messageReceived(MT_UserInfo, &pHeader->userId, lpszData);
		break;
	case MT_UserAction:
		break;
	case MT_Acknowledge:
		//	remove message from pending list
		removePendingMsg(lpszData->toLongLong());
		break;
	case MT_FileReq:
	case MT_FileOp:
		processFile(pHeader, lpszData);
		break;
    default:
        break;
	}
}

void lmcMessaging::processFile(MessageHeader* pHeader, QString* lpszData) {
	QString sendData;
	QStringList fileData = lpszData->split(DELIMITER, QString::SkipEmptyParts);
	int fileOp = fileData[FD_Op].toInt();

	switch(fileOp) {
	case FO_Request:
		emit messageReceived(MT_FileReq, &pHeader->userId, lpszData);
		break;
	case FO_Accept:
		pNetwork->fileOperation(FM_Send, &pHeader->userId, lpszData);
		emit messageReceived(MT_FileOp, &pHeader->userId, lpszData);
		break;
	case FO_CancelSend:
		pNetwork->fileOperation(FM_Receive, &pHeader->userId, lpszData);
		emit messageReceived(MT_FileOp, &pHeader->userId, lpszData);
		break;
	case FO_CancelReceive:
		pNetwork->fileOperation(FM_Send, &pHeader->userId, lpszData);
		emit messageReceived(MT_FileOp, &pHeader->userId, lpszData);
		break;
	case FO_AbortSend:
		pNetwork->fileOperation(FM_Receive, &pHeader->userId, lpszData);
		emit messageReceived(MT_FileOp, &pHeader->userId, lpszData);
		break;
	case FO_AbortReceive:
		pNetwork->fileOperation(FM_Send, &pHeader->userId, lpszData);
		emit messageReceived(MT_FileOp, &pHeader->userId, lpszData);
		break;
	default:
		emit messageReceived(MT_FileOp, &pHeader->userId, lpszData);
		break;
	}
}
