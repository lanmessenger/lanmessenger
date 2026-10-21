// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "udpnetwork.h"
#include <QUdpSocket>
#include <QNetworkAddressEntry>
#include "core/lmccategories.h"
#include "core/shared.h"
#include "core/settings.h"
#include "core/crypto.h"

lmcUdpNetwork::lmcUdpNetwork(void) {
  pUdpReceiver = new QUdpSocket(this);
  pUdpSender = new QUdpSocket(this);
  localId = QString();
  canReceive = false;
  isRunning = false;
  pCrypto = NULL;
  ipAddress = QHostAddress::Any;
  subnetMask = QHostAddress::Any;
  defBroadcast = QHostAddress::Broadcast;
  broadcastList.clear();
}

lmcUdpNetwork::~lmcUdpNetwork(void) {
}

void lmcUdpNetwork::init(int nPort) {
  pSettings = new lmcSettings();
  nUdpPort = nPort;
  multicastAddress = QHostAddress(pSettings->value(IDS_MULTICAST, IDS_MULTICAST_VAL).toString());
  int size = pSettings->beginReadArray(IDS_BROADCASTHDR);
  for(int index = 0; index < size; index++) {
    pSettings->setArrayIndex(index);
    QHostAddress address = QHostAddress(pSettings->value(IDS_BROADCAST).toString());
    if(!broadcastList.contains(address))
      broadcastList.append(address);
  }
  pSettings->endArray();
}

void lmcUdpNetwork::start(void) {
  //	start receiving datagrams
  canReceive = startReceiving();
  isRunning = true;
}

void lmcUdpNetwork::stop(void) {
  disconnect(pUdpReceiver, &QIODevice::readyRead, this, &lmcUdpNetwork::processPendingDatagrams);
  if(pUdpReceiver->state() == QAbstractSocket::BoundState) {
    qCInfo(lmcNetUdp(), "%s",
           qPrintable("Leaving multicast group " + multicastAddress.toString() + " on interface " +
                      multicastInterface.humanReadableName()));
    bool left = pUdpReceiver->leaveMulticastGroup(multicastAddress, multicastInterface);
    pUdpReceiver->close();
    qCInfo(lmcNetUdp(), "%s", (left ? "Success" : "Failed"));
  }
  isRunning = false;
}

void lmcUdpNetwork::setLocalId(QString* lpszLocalId) {
  localId = *lpszLocalId;
}

void lmcUdpNetwork::setCrypto(lmcCrypto* pCrypto) {
  this->pCrypto = pCrypto;
}

void lmcUdpNetwork::sendBroadcast(QString* lpszData) {
  if(!isRunning) {
    qCWarning(lmcNetUdp(), "UDP server not running. Broadcast not sent");
    return;
  }

  QByteArray datagram = lpszData->toUtf8();
  sendDatagram(multicastAddress, datagram);
  for(int index = 0; index < broadcastList.count(); index++) {
    sendDatagram(broadcastList.at(index), datagram);
  }
}

void lmcUdpNetwork::settingsChanged(void) {
  QHostAddress address = QHostAddress(pSettings->value(IDS_MULTICAST, IDS_MULTICAST_VAL).toString());
  if(multicastAddress != address) {
    if(pUdpReceiver->state() == QAbstractSocket::BoundState) {
      qCInfo(lmcNetUdp(), "%s",
             qPrintable("Leaving multicast group " + multicastAddress.toString() + " on interface " +
                        multicastInterface.humanReadableName()));
      bool left = pUdpReceiver->leaveMulticastGroup(multicastAddress, multicastInterface);
      qCInfo(lmcNetUdp(), "%s", (left ? "Success" : "Failed"));
    }
    multicastAddress = address;
    qCInfo(lmcNetUdp(), "%s",
           qPrintable("Joining multicast group " + multicastAddress.toString() + " on interface " +
                      multicastInterface.humanReadableName()));
    bool joined = pUdpReceiver->joinMulticastGroup(multicastAddress, multicastInterface);
    qCInfo(lmcNetUdp(), "%s", (joined ? "Success" : "Failed"));
  }
  broadcastList.clear();
  broadcastList.append(defBroadcast);
  int size = pSettings->beginReadArray(IDS_BROADCASTHDR);
  for(int index = 0; index < size; index++) {
    pSettings->setArrayIndex(index);
    QHostAddress address = QHostAddress(pSettings->value(IDS_BROADCAST).toString());
    if(!broadcastList.contains(address))
      broadcastList.append(address);
  }
  pSettings->endArray();
}

void lmcUdpNetwork::setMulticastInterface(const QNetworkInterface& networkInterface) {
  multicastInterface = networkInterface;
}

void lmcUdpNetwork::setIPAddress(const QString& szAddress, const QString& szSubnet) {
  ipAddress = QHostAddress(szAddress);
  subnetMask = QHostAddress(szSubnet);
  setDefaultBroadcast();
  if(!broadcastList.contains(defBroadcast))
    broadcastList.append(defBroadcast);
}

void lmcUdpNetwork::processPendingDatagrams(void) {
  while(pUdpReceiver->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(pUdpReceiver->pendingDatagramSize());
    QHostAddress address;
    pUdpReceiver->readDatagram(datagram.data(), datagram.size(), &address);
    QString szAddress = address.toString();
    parseDatagram(&szAddress, datagram);
  }
}

void lmcUdpNetwork::sendDatagram(QHostAddress remoteAddress, QByteArray& datagram) {
  if(!isRunning)
    return;

  qCDebug(lmcNetUdp(), "%s",
          qPrintable("Sending UDP datagram to " + remoteAddress.toString() + ":" + QString::number(nUdpPort)));
  pUdpSender->writeDatagram(datagram.data(), datagram.size(), remoteAddress, nUdpPort);
}

bool lmcUdpNetwork::startReceiving(void) {
  qCInfo(lmcNetUdp(), "%s", qPrintable("Binding UDP listener to port " + QString::number(nUdpPort)));

  if(pUdpReceiver->bind(QHostAddress::AnyIPv4, nUdpPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
    qCInfo(lmcNetUdp(), "Success");
    qCInfo(lmcNetUdp(), "%s",
           qPrintable("Joining multicast group " + multicastAddress.toString() +
                      " on interface " + multicastInterface.humanReadableName()));
    bool joined = pUdpReceiver->joinMulticastGroup(multicastAddress, multicastInterface);
    qCInfo(lmcNetUdp(), "%s", (joined ? "Success" : "Failed"));
    connect(pUdpReceiver, &QIODevice::readyRead, this, &lmcUdpNetwork::processPendingDatagrams);
    return true;
  }

  qCCritical(lmcNetUdp(), "Failed");
  return false;
}

void lmcUdpNetwork::parseDatagram(QString* lpszAddress, QByteArray& baDatagram) {
  qCDebug(lmcNetUdp(), "%s", qPrintable("UDP datagram received from " + *lpszAddress));
  DatagramHeader* pHeader = new DatagramHeader(DT_Broadcast, QString(), *lpszAddress);
  QString szData = QString::fromUtf8(baDatagram.data(), baDatagram.length());
  emit broadcastReceived(pHeader, &szData);
}

void lmcUdpNetwork::setDefaultBroadcast(void) {
  if(ipAddress.protocol() != QAbstractSocket::IPv4Protocol)
    return;

  //	The network broadcast address is obtained by ORing the host address and
  //	bit inversed subnet mask
  quint32 ipv4 = ipAddress.toIPv4Address();
  quint32 invMask = ~(subnetMask.toIPv4Address());
  defBroadcast = QHostAddress((ipv4 | invMask));
}
