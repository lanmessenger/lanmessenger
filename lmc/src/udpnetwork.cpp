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
#include "udpnetwork.h"

lmcUdpNetwork::lmcUdpNetwork(void) {
	pUdpSocket = new QUdpSocket(this);
	localId = QString::null;
	canReceive = false;
	isRunning = false;
	pCrypto = NULL;
	listenAddress = QHostAddress::Any;
}

lmcUdpNetwork::~lmcUdpNetwork(void) {
}

void lmcUdpNetwork::init(void) {
	pSettings = new lmcSettings();
	nUdpPort = pSettings->value(IDS_UDPPORT, IDS_UDPPORT_VAL).toInt();
	broadcastAddress = QHostAddress(pSettings->value(IDS_MULTICAST, IDS_MULTICAST_VAL).toString());
}

void lmcUdpNetwork::start(void) {
	//	start receiving datagrams
	canReceive = startReceiving();
	isRunning = true;
}

void lmcUdpNetwork::stop(void) {
	disconnect(pUdpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
	pUdpSocket->leaveMulticastGroup(broadcastAddress, multicastInterface);
	isRunning = false;
}

void lmcUdpNetwork::setLocalId(QString* lpszLocalId) {
	localId = *lpszLocalId;
}

void lmcUdpNetwork::setCrypto(lmcCrypto* pCrypto) {
	this->pCrypto = pCrypto;
}

void lmcUdpNetwork::sendBroadcast(QString* lpszData) {
	QByteArray datagram = lpszData->toUtf8();
	sendDatagram(broadcastAddress, datagram);
}

void lmcUdpNetwork::settingsChanged(void) {
	QHostAddress address = QHostAddress(pSettings->value(IDS_MULTICAST, IDS_MULTICAST_VAL).toString());
	if(broadcastAddress != address) {
		lmcTrace::write("Leaving multicast group " + broadcastAddress.toString() + " on interface " +
			multicastInterface.humanReadableName());
		bool left = pUdpSocket->leaveMulticastGroup(broadcastAddress, multicastInterface);
		lmcTrace::write((left ? "Success" : "Failed"));
		broadcastAddress = address;
		lmcTrace::write("Joining multicast group " + broadcastAddress.toString() + " on interface " +
			multicastInterface.humanReadableName());
		bool joined = pUdpSocket->joinMulticastGroup(broadcastAddress, multicastInterface);
		lmcTrace::write((joined ? "Success" : "Failed"));
	}
}

void lmcUdpNetwork::setMulticastInterface(const QNetworkInterface& networkInterface) {
	multicastInterface = networkInterface;
}

void lmcUdpNetwork::setListenAddress(const QString& szAddress) {
	listenAddress = QHostAddress(szAddress);
}

void lmcUdpNetwork::processPendingDatagrams(void) {
	while(pUdpSocket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(pUdpSocket->pendingDatagramSize());
		QHostAddress address;
		pUdpSocket->readDatagram(datagram.data(), datagram.size(), &address);
        QString szAddress = address.toString();
        parseDatagram(&szAddress, datagram);
	}
}

void lmcUdpNetwork::sendDatagram(QHostAddress remoteAddress, QByteArray& datagram) {
	if(!isRunning)
		return;

	lmcTrace::write("Sending UDP datagram to " + remoteAddress.toString() + ":" + QString::number(nUdpPort));
	pUdpSocket->writeDatagram(datagram.data(), datagram.size(), remoteAddress, nUdpPort);
}

bool lmcUdpNetwork::startReceiving(void) {
	lmcTrace::write("Binding UDP listener to address " + listenAddress.toString() + ":" +
		QString::number(nUdpPort));

	if(pUdpSocket->bind(listenAddress, nUdpPort, QUdpSocket::ShareAddress)) {
		lmcTrace::write("Success\nSetting outgoing interface: " + multicastInterface.humanReadableName());
		pUdpSocket->setMulticastInterface(multicastInterface);
		lmcTrace::write("Joining multicast group " + broadcastAddress.toString() +
			" on interface " + multicastInterface.humanReadableName());
		bool joined = pUdpSocket->joinMulticastGroup(broadcastAddress, multicastInterface);
		lmcTrace::write((joined ? "Success" : "Failed"));
		connect(pUdpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
		return true;
	}

	lmcTrace::write("Failed");
	return false;
}

void lmcUdpNetwork::parseDatagram(QString* lpszAddress, QByteArray& baDatagram) {
	lmcTrace::write("UDP datagram received from " + *lpszAddress);
	DatagramHeader* pHeader = new DatagramHeader(DT_Broadcast, QString(), *lpszAddress);
	QString szData = QString::fromUtf8(baDatagram.data(), baDatagram.length());
	emit broadcastReceived(pHeader, &szData);
}
