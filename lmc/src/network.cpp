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
#include "network.h"

lmcNetwork::lmcNetwork(void) {
	pUdpNetwork = new lmcUdpNetwork();
	pTcpNetwork = new lmcTcpNetwork();
	pWebNetwork = new lmcWebNetwork();
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
	connect(pWebNetwork, SIGNAL(messageReceived(QString*)),
		this, SLOT(web_receiveMessage(QString*)));
	pTimer = NULL;
	pCrypto = new lmcCrypto();
	ipAddress = QString::null;
	subnetMask = QString::null;
	networkInterface = QNetworkInterface();
    interfaceName = QString::null;
	isConnected = false;
	canReceive = false;
}

lmcNetwork::~lmcNetwork(void) {
}

void lmcNetwork::init(XmlMessage *pInitParams) {
	lmcTrace::write("Network initialized");

	pSettings = new lmcSettings();
	isConnected = getIPAddress();

    lmcTrace::write("Network interface selected: " + (networkInterface.isValid() ? networkInterface.humanReadableName() : "None") +
                    "\nIP address obtained: " + (ipAddress.isEmpty() ? "NULL" : ipAddress) +
					"\nSubnet mask obtained: " + (subnetMask.isEmpty() ? "NULL" : subnetMask) +
					"\nConnection status: " + (isConnected ? "OK" : "Fail"));

	int port = pInitParams->data(XN_PORT).toInt();
	pUdpNetwork->init(port);
	pTcpNetwork->init(port);
}

void lmcNetwork::start(void) {
	lmcTrace::write("Network started");
	pCrypto->generateRSA();

	pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
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

	lmcTrace::write("Network stopped");
}

QString lmcNetwork::physicalAddress(void) {
    if(networkInterface.isValid())
        return networkInterface.hardwareAddress();

    return QString::null;

//	//	get the first active network interface
//	QNetworkInterface networkInterface;

//	if(getNetworkInterface(&networkInterface))
//		return networkInterface.hardwareAddress();

//	return QString::null;
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

void lmcNetwork::sendWebMessage(QString *lpszUrl, QString *lpszData) {
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
        lmcTrace::write("Network interface selected: " + (networkInterface.isValid() ? networkInterface.humanReadableName() : "None") +
            "\nIP address obtained: " + (ipAddress.isEmpty() ? "NULL" : ipAddress) +
			"\nSubnet mask obtained: " + (subnetMask.isEmpty() ? "NULL" : subnetMask) +
			"\nConnection status: " + (isConnected ? "OK" : "Fail"));

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

void lmcNetwork::web_receiveMessage(QString *lpszData) {
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
		ipAddress = QString::null;
		subnetMask = QString::null;
		return false;
	}

    // Currently, not using preferred connection, since using preferred connection is not
    // working properly.
	// Get the preferred interface name from settings if checking for the first time
    //if(szInterfaceName.isNull())
    //	szInterfaceName = pSettings->value(IDS_CONNECTION, IDS_CONNECTION_VAL).toString();

	//bool usePreferred = (szInterfaceName.compare(IDS_CONNECTION_VAL, Qt::CaseInsensitive) != 0);
	bool usePreferred = false;

    lmcTrace::write("Checking for active network interface...", verbose);

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
            lmcTrace::write("Active network interface found: " + allInterfaces[index].humanReadableName(),
                verbose);
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

    lmcTrace::write(QString("Warning: ") + (activeFound ? "No IP address found" : "No active network interface found"),
        verbose);
	ipAddress = QString::null;
	subnetMask = QString::null;
	return false;
}

bool lmcNetwork::getIPAddress(QNetworkInterface* pNetworkInterface, QNetworkAddressEntry *pAddressEntry) {
	//lmcTrace::write("Querying IP address from network interface...");

	//	get a list of all associated ip addresses of the interface
	QList<QNetworkAddressEntry> addressEntries = pNetworkInterface->addressEntries();
	//	return the first address which is an ipv4 address
	for(int index = 0; index < addressEntries.count(); index++) {
		if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv4Protocol) {
			*pAddressEntry = addressEntries[index];
			//lmcTrace::write("IPv4 address found for network interface.");
			return true;
		}
	}
	// if ipv4 address is not present, check for ipv6 address
	for(int index = 0; index < addressEntries.count(); index++) {
		if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv6Protocol) {
			*pAddressEntry = addressEntries[index];
			//lmcTrace::write("IPv6 address found for network interface.");
			return true;
		}
	}

	//lmcTrace::write("Warning: No IP address found for network interface.");
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
	lmcTrace::write("Checking for active network interface...");

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
			lmcTrace::write("Active network interface found: " + pNetworkInterface->humanReadableName());
			return true;
		}
	}

	lmcTrace::write("Warning: No active network interface found");
	return false;
}

bool lmcNetwork::isInterfaceUp(QNetworkInterface* pNetworkInterface) {
	if(pNetworkInterface->flags().testFlag(QNetworkInterface::IsUp)
		&& pNetworkInterface->flags().testFlag(QNetworkInterface::IsRunning)
		&& !pNetworkInterface->flags().testFlag(QNetworkInterface::IsLoopBack)) {
			return true;
	}

	return false;
}

bool lmcNetwork::getNetworkAddressEntry(QNetworkAddressEntry* pAddressEntry) {
	//	get the first active network interface
	QNetworkInterface networkInterface;

	if(getNetworkInterface(&networkInterface)) {
		//lmcTrace::write("Querying IP address from network interface...");

		//	get a list of all associated ip addresses of the interface
		QList<QNetworkAddressEntry> addressEntries = networkInterface.addressEntries();
		//	return the first address which is an ipv4 address
		for(int index = 0; index < addressEntries.count(); index++) {
			if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv4Protocol) {
				*pAddressEntry = addressEntries[index];
				this->networkInterface = networkInterface;
                this->interfaceName = networkInterface.name();
				//lmcTrace::write("IPv4 address found for network interface.");
				return true;
			}
		}
		// if ipv4 address is not present, check for ipv6 address
		for(int index = 0; index < addressEntries.count(); index++) {
			if(addressEntries[index].ip().protocol() == QAbstractSocket::IPv6Protocol) {
				*pAddressEntry = addressEntries[index];
				this->networkInterface = networkInterface;
                this->interfaceName = networkInterface.name();
				//lmcTrace::write("IPv6 address found for network interface.");
				return true;
			}
		}

		//lmcTrace::write("Warning: No IP address found for network interface.");
	}

	return false;
}
