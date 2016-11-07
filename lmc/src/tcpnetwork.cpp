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


#include "tcpnetwork.h"

lmcTcpNetwork::lmcTcpNetwork(void) {
	sendList.clear();
	receiveList.clear();
	messageMap.clear();
	locMsgStream = NULL;
	crypto = NULL;
	server = new QTcpServer(this);
	connect(server, SIGNAL(newConnection()), this, SLOT(server_newConnection()));
}

void lmcTcpNetwork::init(void) {
	pSettings = new lmcSettings();
	tcpPort = pSettings->value(IDS_TCPPORT, IDS_TCPPORT_VAL).toInt();
}

void lmcTcpNetwork::start(void) {
	server->listen(QHostAddress::Any, tcpPort);
	isRunning = true;
}

void lmcTcpNetwork::stop(void) {
	server->close();
	isRunning = false;
}

void lmcTcpNetwork::setLocalId(QString* lpszLocalId) {
	localId = *lpszLocalId;
}

void lmcTcpNetwork::setCrypto(lmcCrypto* pCrypto) {
	crypto = pCrypto;
}

void lmcTcpNetwork::addConnection(QString* lpszUserId, QString* lpszAddress) {
	MsgStream* msgStream = new MsgStream(localId, *lpszUserId, *lpszAddress, tcpPort);
	connect(msgStream, SIGNAL(connectionLost(QString*)), 
		this, SLOT(msgStream_connectionLost(QString*)));
	connect(msgStream, SIGNAL(messageReceived(QString*, QString*, QByteArray&)),
		this, SLOT(receiveMessage(QString*, QString*, QByteArray&)));
	
	//	if connecting to own machine, this stream will be stored in local message stream, else in list
	if(lpszUserId->compare(localId) == 0)
		locMsgStream = msgStream;
	else
		messageMap.insert(*lpszUserId, msgStream);
	msgStream->init();
}

void lmcTcpNetwork::sendMessage(QString* lpszReceiverId, QString* lpszData) {
	MsgStream* msgStream;

	if(lpszReceiverId->compare(localId) == 0)
		msgStream = locMsgStream;
	else
		msgStream = messageMap.value(*lpszReceiverId);

	if(msgStream) {
		QByteArray clearData = lpszData->toUtf8();
		QByteArray cipherData = crypto->encrypt(lpszReceiverId, clearData);
		//	cipherData should now contain encrypted content
		QByteArray datagram = Datagram::addHeader(DT_Message, &localId, cipherData);
		msgStream->sendMessage(datagram);
	}
}

void lmcTcpNetwork::initSendFile(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData) {
	QStringList fileData = lpszData->split(DELIMITER, QString::SkipEmptyParts);

	FileSender* sender = new FileSender(fileData[FD_Id], *lpszReceiverId, fileData[FD_Path], fileData[FD_Name], 
		fileData[FD_Size].toLongLong(), *lpszAddress, tcpPort);
	connect(sender, SIGNAL(progressUpdated(FileOp, QString*, QString*, QString*)),
		this, SLOT(update(FileOp, QString*, QString*, QString*)));
	sendList.prepend(sender);
}

void lmcTcpNetwork::initReceiveFile(QString* lpszSenderId, QString* lpszAddress, QString* lpszData) {
	QStringList fileData = lpszData->split(DELIMITER, QString::SkipEmptyParts);

	FileReceiver* receiver = new FileReceiver(fileData[FD_Id], *lpszSenderId, fileData[FD_Path], fileData[FD_Name],
		fileData[FD_Size].toLongLong(), *lpszAddress, tcpPort);
	connect(receiver, SIGNAL(progressUpdated(FileOp, QString*, QString*, QString*)),
		this, SLOT(update(FileOp, QString*, QString*, QString*)));
	receiveList.prepend(receiver);
}

void lmcTcpNetwork::fileOperation(FileMode mode, QString* lpszUserId, QString* lpszData) {
    Q_UNUSED(lpszUserId);

	QStringList fileData = lpszData->split(DELIMITER, QString::SkipEmptyParts);
	int fileOp = fileData[FD_Op].toInt();
	QString id = fileData[FD_Id];

	if(mode == FM_Send) {
		FileSender* sender = getSender(id);
		if(!sender)
			return;

		switch(fileOp) {
		case FO_CancelSend:
		case FO_CancelReceive:
		case FO_AbortReceive:	
			sender->stop();
			break;
		case FO_Accept:
			sender->init();
			break;
		}
	} else {
		FileReceiver* receiver = getReceiver(id);
		if(!receiver)
			return;

		switch(fileOp) {
		case FO_CancelSend:
		case FO_CancelReceive:
		case FO_AbortSend:	
			receiver->stop();
			break;
		}
	}
}

void lmcTcpNetwork::settingsChanged(void) {
}

void lmcTcpNetwork::server_newConnection(void) {
	QTcpSocket* socket = server->nextPendingConnection();
	connect(socket, SIGNAL(readyRead()), this, SLOT(socket_readyRead()));
}

void lmcTcpNetwork::socket_readyRead(void) {
	QTcpSocket* socket = (QTcpSocket*)sender();
	disconnect(socket, SIGNAL(readyRead()), this, SLOT(socket_readyRead()));

	QByteArray buffer = socket->read(64);
	if(buffer.startsWith("MSG")) {
		//	read user id from socket and assign socket to correct message stream
		QString userId(buffer.mid(3)); // 3 is length of "MSG"
		addMsgSocket(&userId, socket);
	} else if(buffer.startsWith("FILE")) {
		//	read transfer id from socket and assign socket to correct file receiver
		QString id(buffer.mid(4)); // 4 is length of "FILE"
		addFileSocket(&id, socket);
	}
}

void lmcTcpNetwork::msgStream_connectionLost(QString* lpszUserId) {
	emit connectionLost(lpszUserId);
}

void lmcTcpNetwork::update(FileOp op, QString* lpszId, QString* lpszUserId, QString* lpszData) {
	QString fileData;

	switch(op) {
	case FO_CompleteSend:
	case FO_CompleteReceive:
	case FO_ErrorSend:
	case FO_ErrorReceive:
		fileData.append(QString::number(op) + DELIMITER + *lpszId + DELIMITER + *lpszData);
		emit progressReceived(lpszUserId, &fileData);
		break;
	case FO_ProgressSend:
	case FO_ProgressReceive:
		fileData.append(QString::number(op) + DELIMITER + *lpszId + DELIMITER + "<path>"
			+ DELIMITER + "<name>" + DELIMITER + *lpszData);
		emit progressReceived(lpszUserId, &fileData);
		break;
    default:
        break;
	}
}

void lmcTcpNetwork::receiveMessage(QString* lpszUserId, QString* lpszAddress, QByteArray& datagram) {
	DatagramHeader* pHeader = NULL;
	if(!Datagram::getHeader(datagram, &pHeader))
		return;
	
	pHeader->address = *lpszAddress;
	QByteArray cipherData = Datagram::getData(datagram);
    QByteArray clearData;
	QString szData;

	switch(pHeader->type) {
	case DT_Broadcast:
		//	send a session key back
		sendSessionKey(lpszUserId, cipherData);
		break;
	case DT_Handshake:
		// decrypt aes key and iv with private key
		crypto->retreiveAES(&pHeader->userId, cipherData);
		emit newConnection(&pHeader->userId, &pHeader->address);
		break;
	case DT_Message:
		// decrypt message with aes
        clearData = crypto->decrypt(&pHeader->userId, cipherData);
		szData = QString::fromUtf8(clearData.data(), clearData.length());
		emit messageReceived(pHeader, &szData);
		break;
    default:
        break;
	}
}

void lmcTcpNetwork::addFileSocket(QString* lpszId, QTcpSocket* pSocket) {
	FileReceiver* receiver = getReceiver(*lpszId);
	if(receiver)
		receiver->init(pSocket);
}

void lmcTcpNetwork::addMsgSocket(QString* lpszUserId, QTcpSocket* pSocket) {
	QString address = pSocket->peerAddress().toString();
	MsgStream* msgStream = new MsgStream(localId, *lpszUserId, address, tcpPort);
	connect(msgStream, SIGNAL(connectionLost(QString*)), 
		this, SLOT(msgStream_connectionLost(QString*)));
	connect(msgStream, SIGNAL(messageReceived(QString*, QString*, QByteArray&)),
		this, SLOT(receiveMessage(QString*, QString*, QByteArray&)));
	messageMap.insert(*lpszUserId, msgStream);
	msgStream->init(pSocket);

	sendPublicKey(lpszUserId);
}

//	Once a new incoming connection is established, the server sends a public key to client
void lmcTcpNetwork::sendPublicKey(QString* lpszUserId) {
	MsgStream* msgStream = messageMap.value(*lpszUserId);
	if(msgStream) {
		QByteArray publicKey = crypto->publicKey;
		QByteArray datagram = Datagram::addHeader(DT_Broadcast, &localId, publicKey);
		msgStream->sendMessage(datagram);
	}
}

//	Once the public key from server is received, the client sends a session key which is
//	encrypted with the public key of the server
void lmcTcpNetwork::sendSessionKey(QString* lpszUserId, QByteArray& publicKey) {
	MsgStream* msgStream;

	if(lpszUserId->compare(localId) == 0)
		msgStream = locMsgStream;
	else
		msgStream = messageMap.value(*lpszUserId);

	if(msgStream) {
		QByteArray sessionKey = crypto->generateAES(lpszUserId, publicKey);
		QByteArray datagram = Datagram::addHeader(DT_Handshake, &localId, sessionKey);
		msgStream->sendMessage(datagram);
	}
}

FileSender* lmcTcpNetwork::getSender(QString id) {
	for(int index = 0; index < sendList.count(); index++)
		if(sendList[index]->id.compare(id) == 0)
			return sendList[index];
	
	return NULL;
}

FileReceiver* lmcTcpNetwork::getReceiver(QString id) {
	for(int index = 0; index < receiveList.count(); index++)
		if(receiveList[index]->id.compare(id) == 0)
			return receiveList[index];
	
	return NULL;
}
