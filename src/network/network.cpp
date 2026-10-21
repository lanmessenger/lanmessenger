// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "network.h"
#include <QTimer>
#include <QNetworkAddressEntry>
#include <QHostAddress>
#include "core/lmccategories.h"
#include "core/shared.h"
#include "core/settings.h"
#include "core/crypto.h"
#include "messaging/xmlmessage.h"
#include "udpnetwork.h"
#include "tcpnetwork.h"
#include "webnetwork.h"

lmcNetwork::lmcNetwork(void) {
  pUdpNetwork = new lmcUdpNetwork();
  pTcpNetwork = new lmcTcpNetwork();
  pWebNetwork = new lmcWebNetwork();
  connect(pUdpNetwork, &lmcUdpNetwork::broadcastReceived, this, &lmcNetwork::udp_receiveBroadcast);
  connect(pTcpNetwork, &lmcTcpNetwork::newConnection, this, &lmcNetwork::tcp_newConnection);
  connect(pTcpNetwork, &lmcTcpNetwork::connectionLost, this, &lmcNetwork::tcp_connectionLost);
  connect(pTcpNetwork, &lmcTcpNetwork::messageReceived, this, &lmcNetwork::tcp_receiveMessage);
  connect(pTcpNetwork, &lmcTcpNetwork::progressReceived, this, &lmcNetwork::tcp_receiveProgress);
  connect(pWebNetwork, &lmcWebNetwork::messageReceived, this, &lmcNetwork::web_receiveMessage);
  pTimer = NULL;
  pCrypto = new lmcCrypto();
  ipAddress = QString();
  subnetMask = QString();
  networkInterface = QNetworkInterface();
  interfaceName = QString();
  isConnected = false;
  canReceive = false;
}

lmcNetwork::~lmcNetwork(void) {
}

void lmcNetwork::init(XmlMessage* pInitParams) {
  qCInfo(lmcNet(), "Network initialized");

  pSettings = new lmcSettings();
  isConnected = getIPAddress();

  qCInfo(lmcNet(), "%s",
         qPrintable("Network interface selected: " +
                    (networkInterface.isValid() ? networkInterface.humanReadableName() : QStringLiteral("None"))));
  qCInfo(lmcNet(), "%s", qPrintable("IP address obtained: " + (ipAddress.isEmpty() ? QStringLiteral("NULL") : ipAddress)));
  qCInfo(lmcNet(), "%s",
         qPrintable("Subnet mask obtained: " + (subnetMask.isEmpty() ? QStringLiteral("NULL") : subnetMask)));
  qCInfo(lmcNet(), "%s",
         qPrintable("Connection status: " +
                    (isConnected ? QStringLiteral("OK") : QStringLiteral("Fail"))));

  int cliUdpPort = pInitParams->data(XN_ARGUDPPORT).toInt();
  int udpPort = cliUdpPort > 0 ? cliUdpPort : pSettings->value(IDS_UDPPORT, IDS_UDPPORT_VAL).toInt();
  int tcpPort = pSettings->value(IDS_TCPPORT, IDS_TCPPORT_VAL).toString().trimmed().toInt();
  pUdpNetwork->init(udpPort);
  pTcpNetwork->init(tcpPort);
}

void lmcNetwork::start(void) {
  qCInfo(lmcNet(), "Network started");
  pCrypto->generateRSA();

  pTimer = new QTimer(this);
  connect(pTimer, &QTimer::timeout, this, &lmcNetwork::timer_timeout);
  pTimer->start(2000);

  pUdpNetwork->setCrypto(pCrypto);
  pTcpNetwork->setCrypto(pCrypto);
  if(isConnected) {
    pUdpNetwork->setMulticastInterface(networkInterface);
    pUdpNetwork->setIPAddress(ipAddress, subnetMask);
    pUdpNetwork->start();
    pTcpNetwork->setIPAddress(ipAddress);
    pTcpNetwork->start();
    canReceive = pUdpNetwork->canReceive;
  }
}

void lmcNetwork::stop(void) {
  pTimer->stop();

  pUdpNetwork->stop();
  pTcpNetwork->stop();

  qCInfo(lmcNet(), "Network stopped");
}

QString lmcNetwork::physicalAddress(void) {
  if(networkInterface.isValid())
    return networkInterface.hardwareAddress();

  return QString();

  //	//	get the first active network interface
  //	QNetworkInterface networkInterface;

  //	if(getNetworkInterface(&networkInterface))
  //		return networkInterface.hardwareAddress();

  //	return QString();
}

void lmcNetwork::setLocalId(QString* lpszLocalId) {
  pUdpNetwork->setLocalId(lpszLocalId);
  pTcpNetwork->setLocalId(lpszLocalId);
}

void lmcNetwork::sendBroadcast(QString* lpszData) {
  pUdpNetwork->sendBroadcast(lpszData);
}

void lmcNetwork::addConnection(QString* lpszUserId, QString* lpszAddress, int nPort) {
  pTcpNetwork->addConnection(lpszUserId, lpszAddress, nPort);
}

int lmcNetwork::tcpServerPort(void) const {
  return pTcpNetwork->serverPort();
}

void lmcNetwork::sendMessage(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData) {
  Q_UNUSED(lpszAddress);
  pTcpNetwork->sendMessage(lpszReceiverId, lpszData);
}

bool lmcNetwork::canSendTo(QString* lpszReceiverId) {
  return pTcpNetwork->canSendTo(lpszReceiverId);
}

void lmcNetwork::initSendFile(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData, int nPort) {
  pTcpNetwork->initSendFile(lpszReceiverId, lpszAddress, lpszData, nPort);
}

void lmcNetwork::initReceiveFile(QString* lpszSenderId, QString* lpszAddress, QString* lpszData) {
  pTcpNetwork->initReceiveFile(lpszSenderId, lpszAddress, lpszData);
}

void lmcNetwork::fileOperation(FileMode mode, QString* lpszUserId, QString* lpszData) {
  pTcpNetwork->fileOperation(mode, lpszUserId, lpszData);
}

void lmcNetwork::sendWebMessage(QString* lpszUrl, QString* lpszData) {
  pWebNetwork->sendMessage(lpszUrl, lpszData);
}

void lmcNetwork::settingsChanged(void) {
  pUdpNetwork->settingsChanged();
  pTcpNetwork->settingsChanged();
}

void lmcNetwork::timer_timeout(void) {
  bool prev = isConnected;
  isConnected = getIPAddress();

  if(prev != isConnected) {
    qCInfo(lmcNet(), "%s",
           qPrintable("Network interface selected: " +
                      (networkInterface.isValid() ? networkInterface.humanReadableName() : QStringLiteral("None"))));
    qCInfo(lmcNet(), "%s", qPrintable("IP address obtained: " + (ipAddress.isEmpty() ? QStringLiteral("NULL") : ipAddress)));
    qCInfo(lmcNet(), "%s",
           qPrintable("Subnet mask obtained: " + (subnetMask.isEmpty() ? QStringLiteral("NULL") : subnetMask)));
    qCInfo(lmcNet(), "%s",
           qPrintable("Connection status: " +
                      (isConnected ? QStringLiteral("OK") : QStringLiteral("Fail"))));

    if(isConnected) {
      pUdpNetwork->setMulticastInterface(networkInterface);
      pUdpNetwork->setIPAddress(ipAddress, subnetMask);
      pUdpNetwork->start();
      pTcpNetwork->setIPAddress(ipAddress);
      pTcpNetwork->start();
      canReceive = pUdpNetwork->canReceive;
    } else {
      pUdpNetwork->stop();
      pTcpNetwork->stop();
    }
    emit connectionStateChanged();
  }
}

void lmcNetwork::udp_receiveBroadcast(DatagramHeader* pHeader, QString* lpszData) {
  emit broadcastReceived(pHeader, lpszData);
}

void lmcNetwork::tcp_newConnection(QString* lpszUserId, QString* lpszAddress) {
  emit newConnection(lpszUserId, lpszAddress);
}

void lmcNetwork::tcp_connectionLost(QString* lpszUserId) {
  emit connectionLost(lpszUserId);
}

void lmcNetwork::tcp_receiveMessage(DatagramHeader* pHeader, QString* lpszData) {
  emit messageReceived(pHeader, lpszData);
}

void lmcNetwork::tcp_receiveProgress(QString* lpszUserId, QString* lpszData) {
  emit progressReceived(lpszUserId, lpszData);
}

void lmcNetwork::web_receiveMessage(QString* lpszData) {
  emit webMessageReceived(lpszData);
}

bool lmcNetwork::getIPAddress() {
  // If an interface is already being used, get it. Ignore all others
  networkInterface = QNetworkInterface::interfaceFromName(interfaceName);
  if(networkInterface.isValid()) {
    QNetworkAddressEntry addressEntry;
    if(isInterfaceUp(&networkInterface) && getIPAddress(&networkInterface, &addressEntry)) {
      ipAddress = addressEntry.ip().toString();
      subnetMask = addressEntry.netmask().toString();
      return true;
    }
    ipAddress = QString();
    subnetMask = QString();
    return false;
  }

  // Currently, not using preferred connection, since using preferred connection is not
  // working properly.

  qCDebug(lmcNet(), "Checking for active network interface...");

  //	get a list of all network interfaces available in the system
  QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();

  bool activeFound = false;

  //	return the preferred interface if it is active
  for(int index = 0; index < allInterfaces.count(); index++) {
    if(isInterfaceUp(&allInterfaces[index])) {
      activeFound = true;
      qCDebug(lmcNet(), "%s",
              qPrintable("Active network interface found: " + allInterfaces[index].humanReadableName()));
      QNetworkAddressEntry addressEntry;
      if(getIPAddress(&allInterfaces[index], &addressEntry)) {
        ipAddress = addressEntry.ip().toString();
        subnetMask = addressEntry.netmask().toString();
        networkInterface = allInterfaces[index];
        interfaceName = allInterfaces[index].name();
        return true;
      }
    }
  }

  qCDebug(lmcNet(), "%s",
          qPrintable(activeFound ? "No IP address found" : "No active network interface found"));
  ipAddress = QString();
  subnetMask = QString();
  return false;
}

bool lmcNetwork::getIPAddress(QNetworkInterface* pNetworkInterface, QNetworkAddressEntry* pAddressEntry) {
  //	get a list of all associated ip addresses of the interface
  QList<QNetworkAddressEntry> addressEntries = pNetworkInterface->addressEntries();
  //	return the first address which is an ipv4 address
  for(int index = 0; index < addressEntries.count(); index++) {
    if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv4Protocol) {
      *pAddressEntry = addressEntries[index];
      return true;
    }
  }
  // if ipv4 address is not present, check for ipv6 address
  for(int index = 0; index < addressEntries.count(); index++) {
    if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv6Protocol) {
      *pAddressEntry = addressEntries[index];
      return true;
    }
  }

  return false;
}

bool lmcNetwork::getNetworkInterface(QNetworkInterface* pNetworkInterface) {
  // If an interface is already being used, get it. Ignore all others
  if(networkInterface.isValid()) {
    *pNetworkInterface = networkInterface;
    return isInterfaceUp(pNetworkInterface);
  }

  qCDebug(lmcNet(), "Checking for active network interface...");

  //	get a list of all network interfaces available in the system
  QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();

  //	return the first active interface
  for(int index = 0; index < allInterfaces.count(); index++) {
    if(isInterfaceUp(&allInterfaces[index])) {
      *pNetworkInterface = allInterfaces[index];
      qCDebug(lmcNet(), "%s", qPrintable("Active network interface found: " + pNetworkInterface->humanReadableName()));
      return true;
    }
  }

  qCDebug(lmcNet(), "No active network interface found");
  return false;
}

bool lmcNetwork::isInterfaceUp(QNetworkInterface* pNetworkInterface) {
  if(pNetworkInterface->flags().testFlag(QNetworkInterface::IsUp) && pNetworkInterface->flags().testFlag(QNetworkInterface::IsRunning) && !pNetworkInterface->flags().testFlag(QNetworkInterface::IsLoopBack)) {
    return true;
  }

  return false;
}

bool lmcNetwork::getNetworkAddressEntry(QNetworkAddressEntry* pAddressEntry) {
  //	get the first active network interface
  QNetworkInterface networkInterface;

  if(getNetworkInterface(&networkInterface)) {
    //	get a list of all associated ip addresses of the interface
    QList<QNetworkAddressEntry> addressEntries = networkInterface.addressEntries();
    //	return the first address which is an ipv4 address
    for(int index = 0; index < addressEntries.count(); index++) {
      if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv4Protocol) {
        *pAddressEntry = addressEntries[index];
        this->networkInterface = networkInterface;
        this->interfaceName = networkInterface.name();
        return true;
      }
    }
    // if ipv4 address is not present, check for ipv6 address
    for(int index = 0; index < addressEntries.count(); index++) {
      if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv6Protocol) {
        *pAddressEntry = addressEntries[index];
        this->networkInterface = networkInterface;
        this->interfaceName = networkInterface.name();
        return true;
      }
    }
  }

  return false;
}
