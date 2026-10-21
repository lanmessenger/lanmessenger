// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef UDPNETWORK_H
#define UDPNETWORK_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkAddressEntry>
#include <QHostAddress>
#include <QList>
#include "core/shared.h"
#include "datagram.h"
#include "core/settings.h"
#include "core/crypto.h"

class lmcUdpNetwork : public QObject {
  Q_OBJECT

public:
  lmcUdpNetwork(void);
  ~lmcUdpNetwork(void);

  void init(int nPort = 0);
  void start(void);
  void stop(void);
  void setLocalId(QString* lpszLocalId);
  void setCrypto(lmcCrypto* pCrypto);
  void sendBroadcast(QString* lpszData);
  void settingsChanged(void);
  void setMulticastInterface(const QNetworkInterface& networkInterface);
  void setIPAddress(const QString& szAddress, const QString& szSubnet);

  bool isConnected;
  bool canReceive;

signals:
  void broadcastReceived(DatagramHeader* pHeader, QString* lpszData);
  void connectionStateChanged(void);

private slots:
  void processPendingDatagrams(void);

private:
  void sendDatagram(QHostAddress remoteAddress, QByteArray& baDatagram);
  bool startReceiving(void);
  void parseDatagram(QString* lpszAddress, QByteArray& baDatagram);
  void setDefaultBroadcast(void);

  lmcSettings* pSettings;
  QUdpSocket* pUdpReceiver;
  QUdpSocket* pUdpSender;
  lmcCrypto* pCrypto;

  bool isRunning;
  quint16 nUdpPort;
  QHostAddress multicastAddress;
  QString localId;
  QNetworkInterface multicastInterface;
  QHostAddress ipAddress;
  QHostAddress subnetMask;
  QList<QHostAddress> broadcastList;
  QHostAddress defBroadcast;
};

#endif // UDPNETWORK_H
