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
#include "tcpnetwork.h"

lmcTcpNetwork::lmcTcpNetwork(void) {
	sendList.clear();
	receiveList.clear();
	messageMap.clear();
	locMsgStream = NULL;
	crypto = NULL;
	listenAddress = QHostAddress::Null;
	server = new QTcpServer(this);
	connect(server, SIGNAL(newConnection()), this, SLOT(server_newConnection()));
}

void lmcTcpNetwork::init(void) {
	pSettings = new lmcSettings();
	tcpPort = pSettings->value(IDS_TCPPORT, IDS_TCPPORT_VAL).toInt();
}

void lmcTcpNetwork::start(void) {
	lmcTrace::write("Starting TCP server...");
	isRunning = server->listen(QHostAddress::Any, tcpPort);
	lmcTrace::write((isRunning ? "Success" : "Failed"));
}

void lmcTcpNetwork::stop(void) {
	server->close();
	// Close all open sockets
	if(locMsgStream)
		locMsgStream->stop();
	QMap<QString, MsgStream*>::const_iterator index = messageMap.constBegin();
	while(index != messageMap.constEnd()) {
		MsgStream* pMsgStream = index.value();
		if(pMsgStream)
			pMsgStream->stop();
		index++;
	}
	isRunning = false;
}

void lmcTcpNetwork::setLocalId(QString* lpszLocalId) {
	localId = *lpszLocalId;
}

void lmcTcpNetwork::setCrypto(lmcCrypto* pCrypto) {
	crypto = pCrypto;
}

void lmcTcpNetwork::addConnection(QString* lpszUserId, QString* lpszAddress) {
	lmcTrace::write("Connecting to user " + *lpszUserId + " at " + *lpszAddress);

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
		lmcTrace::write("Sending TCP data stream to user " + *lpszReceiverId);
		QByteArray clearData = lpszData->toUtf8();
		QByteArray cipherData = crypto->encrypt(lpszReceiverId, clearData);
		//	cipherData should now contain encrypted content
		Datagram::addHeader(DT_Message, cipherData);
		msgStream->sendMessage(cipherData);
	}
}

void lmcTcpNetwork::initSendFile(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData) {
	XmlMessage xmlMessage(*lpszData);
	int type = Helper::indexOf(FileTypeNames, FT_Max, xmlMessage.data(XN_FILETYPE));

	FileSender* sender = new FileSender(xmlMessage.data(XN_FILEID), *lpszReceiverId, xmlMessage.data(XN_FILEPATH), 
		xmlMessage.data(XN_FILENAME), xmlMessage.data(XN_FILESIZE).toLongLong(), *lpszAddress, tcpPort, (FileType)type);
	connect(sender, SIGNAL(progressUpdated(FileMode, FileOp, FileType, QString*, QString*, QString*)),
		this, SLOT(update(FileMode, FileOp, FileType, QString*, QString*, QString*)));
	sendList.prepend(sender);
}

void lmcTcpNetwork::initReceiveFile(QString* lpszSenderId, QString* lpszAddress, QString* lpszData) {
	XmlMessage xmlMessage(*lpszData);
	int type = Helper::indexOf(FileTypeNames, FT_Max, xmlMessage.data(XN_FILETYPE));

	FileReceiver* receiver = new FileReceiver(xmlMessage.data(XN_FILEID), *lpszSenderId, xmlMessage.data(XN_FILEPATH), 
		xmlMessage.data(XN_FILENAME), xmlMessage.data(XN_FILESIZE).toLongLong(), *lpszAddress, tcpPort, (FileType)type);
	connect(receiver, SIGNAL(progressUpdated(FileMode, FileOp, FileType, QString*, QString*, QString*)),
		this, SLOT(update(FileMode, FileOp, FileType, QString*, QString*, QString*)));
	receiveList.prepend(receiver);
}

void lmcTcpNetwork::fileOperation(FileMode mode, QString* lpszUserId, QString* lpszData) {
    Q_UNUSED(lpszUserId);

	XmlMessage xmlMessage(*lpszData);

	int fileOp = Helper::indexOf(FileOpNames, FO_Max, xmlMessage.data(XN_FILEOP));
	QString id = xmlMessage.data(XN_FILEID);

	if(mode == FM_Send) {
		FileSender* sender = getSender(id);
		if(!sender)
			return;

		switch(fileOp) {
		case FO_Cancel:
		case FO_Abort:	
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
		case FO_Cancel:
		case FO_Abort:	
			receiver->stop();
			break;
		}
	}
}

void lmcTcpNetwork::settingsChanged(void) {
}

void lmcTcpNetwork::setListenAddress(const QString& szAddress) {
	listenAddress = QHostAddress(szAddress);
}

void lmcTcpNetwork::server_newConnection(void) {
	lmcTrace::write("New connection received");
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

void lmcTcpNetwork::update(FileMode mode, FileOp op, FileType type, QString* lpszId, QString* lpszUserId, QString* lpszData) {
	XmlMessage xmlMessage;
	xmlMessage.addHeader(XN_FROM, *lpszUserId);
	xmlMessage.addHeader(XN_TO, localId);
	xmlMessage.addData(XN_MODE, FileModeNames[mode]);
	xmlMessage.addData(XN_FILETYPE, FileTypeNames[type]);
	xmlMessage.addData(XN_FILEOP, FileOpNames[op]);
	xmlMessage.addData(XN_FILEID, *lpszId);

	switch(op) {
	case FO_Complete:
	case FO_Error:
		xmlMessage.addData(XN_FILEPATH, *lpszData);
		break;
	case FO_Progress:
		xmlMessage.addData(XN_FILESIZE, *lpszData);
		break;
    default:
        break;
	}

	QString szMessage = xmlMessage.toString();
	emit progressReceived(lpszUserId, &szMessage);
}

void lmcTcpNetwork::receiveMessage(QString* lpszUserId, QString* lpszAddress, QByteArray& datagram) {
	DatagramHeader* pHeader = NULL;
	if(!Datagram::getHeader(datagram, &pHeader))
		return;
	
	pHeader->userId = *lpszUserId;
	pHeader->address = *lpszAddress;
	QByteArray cipherData = Datagram::getData(datagram);
    QByteArray clearData;
	QString szMessage;

	lmcTrace::write("TCP stream type " + QString::number(pHeader->type) +
					" received from user " + *lpszUserId + " at " + *lpszAddress);

	switch(pHeader->type) {
	case DT_PublicKey:
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
		szMessage = QString::fromUtf8(clearData.data(), clearData.length());
		emit messageReceived(pHeader, &szMessage);
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
	lmcTrace::write("Accepted connection from user " + *lpszUserId);
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
	lmcTrace::write("Sending public key to user " + *lpszUserId);
	MsgStream* msgStream = messageMap.value(*lpszUserId);
	if(msgStream) {
		QByteArray publicKey = crypto->publicKey;
		QString sh = DatagramTypeNames[DT_PublicKey];
		Datagram::addHeader(DT_PublicKey, publicKey);
		msgStream->sendMessage(publicKey);
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
		lmcTrace::write("Sending session key to user " + *lpszUserId);
		QByteArray sessionKey = crypto->generateAES(lpszUserId, publicKey);
		Datagram::addHeader(DT_Handshake, sessionKey);
		msgStream->sendMessage(sessionKey);
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
