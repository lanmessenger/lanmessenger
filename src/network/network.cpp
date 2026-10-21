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
         qPrintable("Network interface selected: " + (networkInterface.isValid() ? networkInterface.humanReadableName() : "None") +
                    "\nIP address obtained: " + (ipAddress.isEmpty() ? "NULL" : ipAddress) +
                    "\nSubnet mask obtained: " + (subnetMask.isEmpty() ? "NULL" : subnetMask) +
                    "\nConnection status: " + (isConnected ? "OK" : "Fail")));

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
  isConnected = getIPAddress(false);

  if(prev != isConnected) {
    qCInfo(lmcNet(), "%s",
           qPrintable("Network interface selected: " + (networkInterface.isValid() ? networkInterface.humanReadableName() : "None") +
                      "\nIP address obtained: " + (ipAddress.isEmpty() ? "NULL" : ipAddress) +
                      "\nSubnet mask obtained: " + (subnetMask.isEmpty() ? "NULL" : subnetMask) +
                      "\nConnection status: " + (isConnected ? "OK" : "Fail")));

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

bool lmcNetwork::getIPAddress(bool verbose) {
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
  // Get the preferred interface name from settings if checking for the first time
  //if(szInterfaceName.isNull())
  //	szInterfaceName = pSettings->value(IDS_CONNECTION, IDS_CONNECTION_VAL).toString();

  //bool usePreferred = (szInterfaceName.compare(IDS_CONNECTION_VAL, Qt::CaseInsensitive) != 0);
  bool usePreferred = false;

  if(verbose) {
    qCDebug(lmcNet(), "Checking for active network interface...");
  }

  //	get a list of all network interfaces available in the system
  QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();

  bool activeFound = false;

  //	return the preferred interface if it is active
  for(int index = 0; index < allInterfaces.count(); index++) {
    // Skip to the next interface if it is not the preferred one
    // Checked only if searching for the preferred adapter
    if(usePreferred && interfaceName.compare(allInterfaces[index].name()) != 0)
      continue;

    if(isInterfaceUp(&allInterfaces[index])) {
      activeFound = true;
      if(verbose) {
        qCInfo(lmcNet(), "%s",
               qPrintable("Active network interface found: " + allInterfaces[index].humanReadableName()));
      }
      QNetworkAddressEntry addressEntry;
      if(getIPAddress(&allInterfaces[index], &addressEntry)) {
        ipAddress = addressEntry.ip().toString();
        subnetMask = addressEntry.netmask().toString();
        networkInterface = allInterfaces[index];
        interfaceName = allInterfaces[index].name();
        usePreferred = false;
        return true;
      }
    }
  }

  if(verbose) {
    qCWarning(lmcNet(), "%s",
              qPrintable(activeFound ? "No IP address found" : "No active network interface found"));
  }
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

  // Get the preferred interface name from settings if checking for the first time
  if(interfaceName.isNull())
    interfaceName = pSettings->value(IDS_CONNECTION, IDS_CONNECTION_VAL).toString();

  QString szPreferred = interfaceName;
  // Currently, hard coding usePreferred to False, since using preferred connection is not
  // working properly.
  //bool usePreferred = (szPreferred.compare(IDS_CONNECTION_VAL, Qt::CaseInsensitive) != 0);
  bool usePreferred = false;

  // Return true if preferred interface is available
  if(usePreferred && getNetworkInterface(pNetworkInterface, &szPreferred))
    return true;

  // Return true if a fallback interface is available
  if(!usePreferred && getNetworkInterface(pNetworkInterface, NULL))
    return true;

  return false;
}

bool lmcNetwork::getNetworkInterface(QNetworkInterface* pNetworkInterface, QString* lpszPreferred) {
  qCDebug(lmcNet(), "Checking for active network interface...");

  //	get a list of all network interfaces available in the system
  QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();

  //	return the preferred interface if it is active
  for(int index = 0; index < allInterfaces.count(); index++) {
    // Skip to the next interface if it is not the preferred one
    // Checked only if searching for the preferred adapter
    if(lpszPreferred && lpszPreferred->compare(allInterfaces[index].name()) != 0)
      continue;

    if(isInterfaceUp(&allInterfaces[index])) {
      *pNetworkInterface = allInterfaces[index];
      qCInfo(lmcNet(), "%s", qPrintable("Active network interface found: " + pNetworkInterface->humanReadableName()));
      return true;
    }
  }

  qCWarning(lmcNet(), "No active network interface found");
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
