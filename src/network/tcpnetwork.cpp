// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QTcpServer>
#include <QTcpSocket>
#include "core/lmccategories.h"
#include "core/shared.h"
#include "core/crypto.h"
#include "tcpnetwork.h"
#include "netstreamer.h"
#include "datagram.h"
#include "messaging/xmlmessage.h"
#include "core/settings.h"

lmcTcpNetwork::lmcTcpNetwork(void) {
  sendList.clear();
  receiveList.clear();
  messageMap.clear();
  locMsgStream = NULL;
  crypto = NULL;
  isRunning = false;
  stoppingForExit = false;
  ipAddress = QHostAddress::Null;
  server = new QTcpServer(this);
  connect(server, &QTcpServer::newConnection, this, &lmcTcpNetwork::server_newConnection);
}

void lmcTcpNetwork::init(int nPort) {
  tcpPort = nPort;
}

void lmcTcpNetwork::start(void) {
  if(stoppingForExit) {
    qCInfo(lmcNetTcp(), "TCP start ignored, stopping for exit");
    return;
  }
  qCInfo(lmcNetTcp(), "Starting TCP server");
  isRunning = server->listen(QHostAddress::Any, tcpPort > 0 ? tcpPort : IDS_TCPPORT_LEGACY_VAL);
  if(!isRunning && tcpPort == 0 && server->serverError() == QAbstractSocket::AddressInUseError) {
    isRunning = server->listen(QHostAddress::Any, 0);
  }
  if(isRunning) {
    int actualPort = server->serverPort();
    qCInfo(lmcNetTcp(), "%s", qPrintable("TCP server listening on port " + QString::number(actualPort)));
  } else {
    qCCritical(lmcNetTcp(), "%s", qPrintable("TCP server listening failed " + server->errorString()));
  }
}

void lmcTcpNetwork::suspend(void) {
  server->close();
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

void lmcTcpNetwork::stop(void) {
  stoppingForExit = true;
  suspend();
  for(int i = 0; i < sendList.count(); i++)
    sendList[i]->stop();
  for(int i = 0; i < receiveList.count(); i++)
    receiveList[i]->stop();
}

void lmcTcpNetwork::setLocalId(QString* lpszLocalId) {
  localId = *lpszLocalId;
}

void lmcTcpNetwork::setCrypto(lmcCrypto* pCrypto) {
  crypto = pCrypto;
}

void lmcTcpNetwork::addConnection(QString* lpszUserId, QString* lpszAddress, int nPort) {
  if(!isRunning) {
    qCWarning(lmcNetTcp(), "TCP server not running. Unable to connect");
    return;
  }

  int peerPort = nPort > 0 ? nPort : IDS_TCPPORT_LEGACY_VAL;
  qCInfo(lmcNetTcp(), "%s",
         qPrintable("Connecting to user " + *lpszUserId + " at " + *lpszAddress + ":" + QString::number(peerPort)));

  MsgStream* msgStream = new MsgStream(localId, *lpszUserId, *lpszAddress, peerPort);
  connect(msgStream, &MsgStream::connectionLost, this, &lmcTcpNetwork::msgStream_connectionLost);
  connect(msgStream, &MsgStream::messageReceived, this, &lmcTcpNetwork::receiveMessage);

  //	if connecting to own machine, this stream will be stored in local message stream, else in list
  if(lpszUserId->compare(localId) == 0)
    locMsgStream = msgStream;
  else
    messageMap.insert(*lpszUserId, msgStream);
  msgStream->init();
}

void lmcTcpNetwork::sendMessage(QString* lpszReceiverId, QString* lpszData) {
  if(!isRunning) {
    qCWarning(lmcNetTcp(), "TCP server not running. Message not sent");
    return;
  }

  MsgStream* msgStream;

  if(lpszReceiverId->compare(localId) == 0)
    msgStream = locMsgStream;
  else
    msgStream = messageMap.value(*lpszReceiverId, NULL);

  if(msgStream) {
    qCDebug(lmcNetTcp(), "%s", qPrintable("Sending TCP data stream to user " + *lpszReceiverId));
    QByteArray clearData = lpszData->toUtf8();
    QByteArray cipherData = crypto->encrypt(lpszReceiverId, clearData);
    if(cipherData.isEmpty()) {
      qCWarning(lmcNetTcp(), "Message could not be sent");
      return;
    }
    //	cipherData should now contain encrypted content
    Datagram::addHeader(DT_Message, cipherData);
    msgStream->sendMessage(cipherData);
    return;
  }

  qCWarning(lmcNetTcp(), "Socket not found. Message sending failed");
}

//	Point-in-time: a dead connection flips this back via removeUser.
bool lmcTcpNetwork::canSendTo(QString* lpszUserId) {
  if(!isRunning)
    return false;
  if(lpszUserId->compare(localId) == 0)
    return locMsgStream != NULL && crypto && crypto->hasSession(lpszUserId);
  if(!messageMap.contains(*lpszUserId))
    return false;
  return crypto && crypto->hasSession(lpszUserId);
}

void lmcTcpNetwork::initSendFile(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData, int nPort) {
  XmlMessage xmlMessage(*lpszData);
  int type = Helper::indexOf(FileTypeNames, FT_Max, xmlMessage.data(XN_FILETYPE));

  int peerPort = nPort > 0 ? nPort : IDS_TCPPORT_LEGACY_VAL;
  qCInfo(lmcNetTcp(), "%s",
         qPrintable("Sending file " + xmlMessage.data(XN_FILEID) + " to user " + *lpszReceiverId + " at " +
                    *lpszAddress + ":" + QString::number(peerPort)));

  FileSender* sender = new FileSender(xmlMessage.data(XN_FILEID), localId, *lpszReceiverId, xmlMessage.data(XN_FILEPATH),
                                      xmlMessage.data(XN_FILENAME), xmlMessage.data(XN_FILESIZE).toLongLong(), *lpszAddress, peerPort, (FileType)type);
  connect(sender, &FileSender::progressUpdated, this, &lmcTcpNetwork::update);
  sendList.prepend(sender);
  sender->init();
}

void lmcTcpNetwork::initReceiveFile(QString* lpszSenderId, QString* lpszAddress, QString* lpszData) {
  XmlMessage xmlMessage(*lpszData);
  int type = Helper::indexOf(FileTypeNames, FT_Max, xmlMessage.data(XN_FILETYPE));

  FileReceiver* receiver = new FileReceiver(xmlMessage.data(XN_FILEID), *lpszSenderId, xmlMessage.data(XN_FILEPATH),
                                            xmlMessage.data(XN_FILENAME), xmlMessage.data(XN_FILESIZE).toLongLong(), *lpszAddress, tcpPort, (FileType)type);
  connect(receiver, &FileReceiver::progressUpdated, this, &lmcTcpNetwork::update);
  receiveList.prepend(receiver);
}

void lmcTcpNetwork::fileOperation(FileMode mode, QString* lpszUserId, QString* lpszData) {
  Q_UNUSED(lpszUserId);

  XmlMessage xmlMessage(*lpszData);

  int fileOp = Helper::indexOf(FileOpNames, FO_Max, xmlMessage.data(XN_FILEOP));
  QString id = xmlMessage.data(XN_FILEID);

  if(mode == FM_Send) {
    FileSender* sender = getSender(id, *lpszUserId);
    if(!sender)
      return;

    switch(fileOp) {
    case FO_Cancel:
    case FO_Abort:
      sender->stop();
      removeSender(sender);
      break;
    }
  } else {
    FileReceiver* receiver = getReceiver(id, *lpszUserId);
    if(!receiver)
      return;

    switch(fileOp) {
    case FO_Cancel:
    case FO_Abort:
      receiver->stop();
      removeReceiver(receiver);
      break;
    }
  }
}

void lmcTcpNetwork::settingsChanged(void) {
}

void lmcTcpNetwork::setIPAddress(const QString& szAddress) {
  ipAddress = QHostAddress(szAddress);
}

int lmcTcpNetwork::serverPort(void) const {
  if(!isRunning)
    return 0;
  int actualPort = server->serverPort();
  return actualPort > 0 ? actualPort : tcpPort;
}

void lmcTcpNetwork::server_newConnection(void) {
  qCInfo(lmcNetTcp(), "New connection received");
  QTcpSocket* socket = server->nextPendingConnection();
  connect(socket, &QIODevice::readyRead, this, &lmcTcpNetwork::socket_readyRead);
}

void lmcTcpNetwork::socket_readyRead(void) {
  QTcpSocket* socket = (QTcpSocket*)sender();
  disconnect(socket, &QIODevice::readyRead, this, &lmcTcpNetwork::socket_readyRead);

  QByteArray buffer = socket->read(64);
  if(buffer.startsWith("MSG")) {
    //	read user id from socket and assign socket to correct message stream
    QString userId(buffer.mid(3)); // 3 is length of "MSG"
    addMsgSocket(&userId, socket);
  } else if(buffer.startsWith("FILE")) {
    //	read transfer id from socket and assign socket to correct file receiver
    QString id(buffer.mid(4, 32)); // 4 is length of "FILE", 32 is length of File Id
    QString userId(buffer.mid(36));
    addFileSocket(&id, &userId, socket);
  }
}

void lmcTcpNetwork::msgStream_connectionLost(QString* lpszUserId) {
  if(stoppingForExit)
    return;
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
    if(mode == FM_Send)
      removeSender(static_cast<FileSender*>(sender()));
    else
      removeReceiver(static_cast<FileReceiver*>(sender()));
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

  qCDebug(lmcNetTcp(), "%s",
          qPrintable("TCP stream type " + QString::number(pHeader->type) +
                     " received from user " + *lpszUserId + " at " + *lpszAddress));

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
    if(clearData.isEmpty()) {
      qCWarning(lmcNetTcp(), "Message could not be retrieved");
      break;
    }
    szMessage = QString::fromUtf8(clearData.data(), clearData.length());
    emit messageReceived(pHeader, &szMessage);
    break;
  default:
    break;
  }
}

void lmcTcpNetwork::addFileSocket(QString* lpszId, QString* lpszUserId, QTcpSocket* pSocket) {
  FileReceiver* receiver = getReceiver(*lpszId, *lpszUserId);
  if(receiver)
    receiver->init(pSocket);
}

void lmcTcpNetwork::addMsgSocket(QString* lpszUserId, QTcpSocket* pSocket) {
  qCInfo(lmcNetTcp(), "%s", qPrintable("Accepted connection from user " + *lpszUserId));
  QString address = pSocket->peerAddress().toString();
  MsgStream* msgStream = new MsgStream(localId, *lpszUserId, address, tcpPort);
  connect(msgStream, &MsgStream::connectionLost, this, &lmcTcpNetwork::msgStream_connectionLost);
  connect(msgStream, &MsgStream::messageReceived, this, &lmcTcpNetwork::receiveMessage);
  messageMap.insert(*lpszUserId, msgStream);
  msgStream->init(pSocket);

  sendPublicKey(lpszUserId);
}

//	Once a new incoming connection is established, the server sends a public key to client
void lmcTcpNetwork::sendPublicKey(QString* lpszUserId) {
  qCInfo(lmcNetTcp(), "%s", qPrintable("Sending public key to user " + *lpszUserId));
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
    qCInfo(lmcNetTcp(), "%s", qPrintable("Sending session key to user " + *lpszUserId));
    QByteArray sessionKey = crypto->generateAES(lpszUserId, publicKey);
    Datagram::addHeader(DT_Handshake, sessionKey);
    msgStream->sendMessage(sessionKey);
  }
}

FileSender* lmcTcpNetwork::getSender(QString id, QString userId) {
  for(int index = 0; index < sendList.count(); index++)
    if(sendList[index]->id.compare(id) == 0 && sendList[index]->peerId.compare(userId) == 0)
      return sendList[index];

  return NULL;
}

FileReceiver* lmcTcpNetwork::getReceiver(QString id, QString userId) {
  for(int index = 0; index < receiveList.count(); index++)
    if(receiveList[index]->id.compare(id) == 0 && receiveList[index]->peerId.compare(userId) == 0)
      return receiveList[index];

  return NULL;
}

void lmcTcpNetwork::removeSender(FileSender* pSender) {
  int index = sendList.indexOf(pSender);
  FileSender* sender = sendList.takeAt(index);
  sender->deleteLater(); // deleting later is generally safer
}

void lmcTcpNetwork::removeReceiver(FileReceiver* pReceiver) {
  int index = receiveList.indexOf(pReceiver);
  FileReceiver* receiver = receiveList.takeAt(index);
  receiver->deleteLater(); // deleting later is generally safer
}
