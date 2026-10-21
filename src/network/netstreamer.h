// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include "core/definitions.h"

class QTcpSocket;
class QTimer;
class QFile;

/****************************************************************************
** Class: FileSender
** Description: Handles sending files.
****************************************************************************/
class FileSender : public QObject {
  Q_OBJECT

public:
  FileSender(void);
  FileSender(QString szId, QString szLocalId, QString szPeerId, QString szFilePath, QString szFileName, qint64 nFileSize,
             QString szAddress, int nPort, FileType nType);
  ~FileSender(void);

  void init(void);
  void stop(void);

  QString id;
  QString peerId;
  FileType type;

signals:
  void progressUpdated(FileMode mode, FileOp fileOp, FileType type, QString* lpszId, QString* lpszUserId, QString* lpszData);

private slots:
  void connected(void);
  void disconnected(void);
  void readyRead(void);
  void bytesWritten(qint64 bytes);
  void timer_timeout(void);

private:
  void sendFile(void);

  QString localId;
  QString filePath;
  QString fileName;
  qint64 fileSize;
  QString address;
  int port;
  qint64 sentBytes;
  QTcpSocket* socket;
  QFile* file;
  char* buffer;
  bool active = false;
  bool finishing = false;
  qint64 milestone;
  qint64 mile;
  QTimer* timer;
};

/****************************************************************************
** Class: FileReceiver
** Description: Handles receiving files.
****************************************************************************/
class FileReceiver : public QObject {
  Q_OBJECT

public:
  FileReceiver(void);
  FileReceiver(QString szId, QString szPeerId, QString szFilePath, QString szFileName, qint64 nFileSize,
               QString szAddress, int nPort, FileType nType);
  ~FileReceiver(void);

  void init(QTcpSocket* socket);
  void stop(void);

  QString id;
  QString peerId;
  FileType type;

signals:
  void progressUpdated(FileMode mode, FileOp fileOp, FileType type, QString* lpszId, QString* lpszUserId, QString* lpszData);

private slots:
  void disconnected(void);
  void readyRead(void);
  void timer_timeout(void);

private:
  void receiveFile(void);

  QString filePath;
  QString fileName;
  qint64 fileSize;
  QString address;
  int port;
  qint64 sentBytes;
  QTcpSocket* socket;
  QFile* file;
  char* buffer;
  bool active = false;
  qint64 milestone;
  qint64 mile;
  QTimer* timer;
  int numTimeOuts;
  qint64 lastPosition;
};

/****************************************************************************
** Class: MsgStream
** Description: Handles transmission and reception of TCP streaming messages.
****************************************************************************/
class MsgStream : public QObject {
  Q_OBJECT

public:
  MsgStream(void);
  MsgStream(QString szLocalId, QString szPeerId, QString szPeerAddress, int nPort);
  ~MsgStream(void);

  void init(void);
  void init(QTcpSocket* socket);
  void stop(void);
  void sendMessage(QByteArray& data);

signals:
  void connectionLost(QString* lpszUserId);
  void messageReceived(QString* lpszUserId, QString* lpszAddress, QByteArray& data);

private slots:
  void connected(void);
  void disconnected(void);
  void readyRead(void);
  void bytesWritten(qint64 bytes);

private:
  QTcpSocket* socket;
  int port;
  QString localId;
  QString peerId;
  QString peerAddress;
  QByteArray outData;
  QByteArray inData;
  quint32 outDataLen;
  quint32 inDataLen;
  bool reading;
};
