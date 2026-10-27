// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QObject>
#include <QString>
#include <QNetworkInterface>
#include "core/definitions.h"

struct DatagramHeader;
class XmlMessage;
class lmcSettings;
class lmcUdpNetwork;
class lmcTcpNetwork;
class lmcWebNetwork;
class lmcCrypto;
class QTimer;
class QNetworkAddressEntry;
class QHostAddress;

class lmcNetwork : public QObject {
  Q_OBJECT

public:
  lmcNetwork(void);
  ~lmcNetwork(void);

  void init(XmlMessage* pInitParams);
  void start(void);
  void suspend(void);
  void stop(void);
  QString physicalAddress(void);
  void setLocalId(QString* lpszLocalId);
  void sendBroadcast(QString* lpszData);
  void addConnection(QString* lpszUserId, QString* lpszAddress, int nPort = 0);
  int tcpServerPort(void) const;
  void sendMessage(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData);
  bool canSendTo(QString* lpszReceiverId);
  void initSendFile(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData, int nPort = 0);
  void initReceiveFile(QString* lpszSenderId, QString* lpszAddress, QString* lpszData);
  void fileOperation(FileMode mode, QString* lpszUserId, QString* lpszData);
  void sendWebMessage(QString* lpszUrl, QString* lpszData);
  void settingsChanged(void);

  QString ipAddress;
  QString subnetMask;
  bool isConnected;
  bool canReceive;

signals:
  void connectionStateChanged(void);
  void broadcastReceived(DatagramHeader* pHeader, QString* lpszData);
  void newConnection(QString* lpszUserId, QString* lpszAddress);
  void connectionLost(QString* lpszUserId);
  void messageReceived(DatagramHeader* pHeader, QString* lpszData);
  void progressReceived(QString* lpszUserId, QString* lpszData);
  void webMessageReceived(QString* lpszData);

private slots:
  void timer_timeout(void);
  void udp_receiveBroadcast(DatagramHeader* pHeader, QString* lpszData);
  void tcp_newConnection(QString* lpszUserId, QString* lpszAddress);
  void tcp_connectionLost(QString* lpszUserId);
  void tcp_receiveMessage(DatagramHeader* pHeader, QString* lpszData);
  void tcp_receiveProgress(QString* lpszUserId, QString* lpszData);
  void web_receiveMessage(QString* lpszData);

private:
  bool getIPAddress();
  bool getIPAddress(QNetworkInterface* pNetworkInterface, QNetworkAddressEntry* pAddressEntry);
  bool getNetworkInterface(QNetworkInterface* pNetworkInterface);
  bool isInterfaceUp(QNetworkInterface* pNetworkInterface);
  bool getNetworkAddressEntry(QNetworkAddressEntry* pAddressEntry);

  struct NetworkAdapter {
    QString name;
    QString description;
    QString type;
  };

  lmcSettings* pSettings;
  lmcUdpNetwork* pUdpNetwork;
  lmcTcpNetwork* pTcpNetwork;
  lmcWebNetwork* pWebNetwork;
  lmcCrypto* pCrypto;
  QTimer* pTimer;
  QString interfaceName;
  QNetworkInterface networkInterface;
};
