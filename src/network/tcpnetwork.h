// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QHostAddress>
#include "core/definitions.h"

class QTcpSocket;
class QTcpServer;
class FileSender;
class FileReceiver;
class MsgStream;
class lmcCrypto;
struct DatagramHeader;

class lmcTcpNetwork : public QObject {
  Q_OBJECT

public:
  lmcTcpNetwork(void);
  ~lmcTcpNetwork(void) {}

  void init(int nPort);
  void start(void);
  void suspend(void);
  void stop(void);
  void setLocalId(QString* lpszLocalId);
  void setCrypto(lmcCrypto* pCrypto);
  void addConnection(QString* lpszUserId, QString* lpszAddress, int nPort = 0);
  void sendMessage(QString* lpszReceiverId, QString* lpszData);
  bool canSendTo(QString* lpszUserId);
  void initSendFile(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData, int nPort = 0);
  void initReceiveFile(QString* lpszSenderId, QString* lpszAddress, QString* lpszData);
  void fileOperation(FileMode mode, QString* lpszUserId, QString* lpszData);
  void settingsChanged(void);
  void setIPAddress(const QString& szAddress);
  int serverPort(void) const;

signals:
  void newConnection(QString* lpszUserId, QString* lpszAddress);
  void connectionLost(QString* lpszUserId);
  void messageReceived(DatagramHeader* pHeader, QString* lpszData);
  void progressReceived(QString* lpszUserId, QString* lpszData);

private slots:
  void server_newConnection(void);
  void socket_readyRead(void);
  void msgStream_connectionLost(QString* lpszUserId);
  void update(FileMode mode, FileOp op, FileType type, QString* lpszId, QString* lpszUserId, QString* lpszData);
  void receiveMessage(QString* lpszUserId, QString* lpszAddress, QByteArray& data);

private:
  void addFileSocket(QString* lpszId, QString* lpszUserId, QTcpSocket* pSocket);
  void addMsgSocket(QString* lpszUserId, QTcpSocket* pSocket);
  void sendPublicKey(QString* lpszUserId);
  void sendSessionKey(QString* lpszUserId, QByteArray& publicKey);
  FileSender* getSender(QString id, QString userId);
  FileReceiver* getReceiver(QString id, QString userId);
  void removeSender(FileSender* pSender);
  void removeReceiver(FileReceiver* pReceiver);

  QTcpServer* server;
  QList<FileSender*> sendList;
  QList<FileReceiver*> receiveList;
  QMap<QString, MsgStream*> messageMap;
  MsgStream* locMsgStream;
  bool isRunning;
  bool stoppingForExit; // terminal, no restart after stop()
  int tcpPort;
  QString localId;
  lmcCrypto* crypto;
  QHostAddress ipAddress;
};
