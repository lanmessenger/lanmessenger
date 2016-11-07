/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
** 
** Contact:  dilipvrk@gmail.com
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


#include "network.h"

lmcNetwork::lmcNetwork(void) {
	pUdpNetwork = new lmcUdpNetwork();
	pTcpNetwork = new lmcTcpNetwork();
	connect(pUdpNetwork, SIGNAL(broadcastReceived(DatagramHeader*, QString*)), 
		this, SLOT(udp_receiveBroadcast(DatagramHeader*, QString*)));
	connect(pTcpNetwork, SIGNAL(newConnection(QString*, QString*)),
		this, SLOT(tcp_newConnection(QString*, QString*)));
	connect(pTcpNetwork, SIGNAL(connectionLost(QString*)),
		this, SLOT(tcp_connectionLost(QString*)));
	connect(pTcpNetwork, SIGNAL(messageReceived(DatagramHeader*, QString*)), 
		this, SLOT(tcp_receiveMessage(DatagramHeader*, QString*)));
	connect(pTcpNetwork, SIGNAL(progressReceived(QString*, QString*)),
		this, SLOT(tcp_receiveProgress(QString*, QString*)));
	pTimer = NULL;
	pCrypto = new lmcCrypto();
	ipAddress = QString::null;
	subnetMask = QString::null;
}

lmcNetwork::~lmcNetwork(void) {
}

void lmcNetwork::init(void) {
	ipAddress = getIPAddress().toString();
	subnetMask = getSubnetMask().toString();
	isConnected = !ipAddress.isNull();

	pUdpNetwork->init();
	pTcpNetwork->init();
}

void lmcNetwork::start(void) {
	pCrypto->generateRSA();

	pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
	pTimer->start(2000);

	pUdpNetwork->setCrypto(pCrypto);
	pTcpNetwork->setCrypto(pCrypto);
	pUdpNetwork->start();
	pTcpNetwork->start();

	canReceive = pUdpNetwork->canReceive;
}

void lmcNetwork::stop(void) {
	pTimer->stop();

	pUdpNetwork->stop();
	pTcpNetwork->stop();
}

QHostAddress lmcNetwork::getIPAddress(void) {
	QNetworkAddressEntry hostAddressEntry;
	
	if(getNetworkAddressEntry(&hostAddressEntry))
		return hostAddressEntry.ip();
	
	return QHostAddress::Null;
}

QHostAddress lmcNetwork::getSubnetMask(void) {
	QNetworkAddressEntry hostAddressEntry;
	
	if(getNetworkAddressEntry(&hostAddressEntry))
		return hostAddressEntry.netmask();
	
	return QHostAddress::Null;
}

QString lmcNetwork::getPhysicalAddress(void) {
	//	get the first active network interface
	QNetworkInterface networkInterface;

	if(getNetworkInterface(&networkInterface))
		return networkInterface.hardwareAddress();

	return QString::null;
}

void lmcNetwork::setLocalId(QString* lpszLocalId) {
	pUdpNetwork->setLocalId(lpszLocalId);
	pTcpNetwork->setLocalId(lpszLocalId);
}

void lmcNetwork::sendBroadcast(QString* lpszData) {
	pUdpNetwork->sendBroadcast(lpszData);
}

void lmcNetwork::addConnection(QString* lpszUserId, QString* lpszAddress) {
	pTcpNetwork->addConnection(lpszUserId, lpszAddress);
}

void lmcNetwork::sendMessage(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData) {
    Q_UNUSED(lpszAddress);
	pTcpNetwork->sendMessage(lpszReceiverId, lpszData);
}

void lmcNetwork::initSendFile(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData) {
	pTcpNetwork->initSendFile(lpszReceiverId, lpszAddress, lpszData);
}

void lmcNetwork::initReceiveFile(QString* lpszSenderId, QString* lpszAddress, QString* lpszData) {
	pTcpNetwork->initReceiveFile(lpszSenderId, lpszAddress, lpszData);
}

void lmcNetwork::fileOperation(FileMode mode, QString* lpszUserId, QString* lpszData) {
	pTcpNetwork->fileOperation(mode, lpszUserId, lpszData);
}

void lmcNetwork::settingsChanged(void) {
	pUdpNetwork->settingsChanged();
	pTcpNetwork->settingsChanged();
}

void lmcNetwork::timer_timeout(void) {
	ipAddress = getIPAddress().toString();
	subnetMask = getSubnetMask().toString();

	bool prev = isConnected;
	isConnected = !ipAddress.isNull();

	if(prev != isConnected)
		emit connectionStateChanged();
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

bool lmcNetwork::getNetworkInterface(QNetworkInterface* pNetworkInterface) {
	//	get a list of all network interfaces available in the system
	QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();

	//	return the first interface which is active
	for(int index = 0; index < allInterfaces.count(); index++)
		if(allInterfaces[index].flags().testFlag(QNetworkInterface::IsUp) 
			&& allInterfaces[index].flags().testFlag(QNetworkInterface::IsRunning)
			&& !allInterfaces[index].flags().testFlag(QNetworkInterface::IsLoopBack)) {
				*pNetworkInterface = allInterfaces[index];
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
		for(int index = 0; index < addressEntries.count(); index++)
			if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv4Protocol) {
				*pAddressEntry = addressEntries[index];
				return true;
			}
	}

	return false;
}
