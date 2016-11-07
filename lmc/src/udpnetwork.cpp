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


#include "udpnetwork.h"

lmcUdpNetwork::lmcUdpNetwork(void) {
	pUdpSender = new QUdpSocket(this);
	pUdpReceiver = new QUdpSocket(this);
	localId = QString::null;
	isRunning = false;
	pCrypto = NULL;
}

lmcUdpNetwork::~lmcUdpNetwork(void) {
}

void lmcUdpNetwork::init(void) {
	pSettings = new lmcSettings();
	nUdpPort = pSettings->value(IDS_UDPPORT, IDS_UDPPORT_VAL).toInt();
	broadcastAddress = QHostAddress(pSettings->value(IDS_BROADCAST, IDS_BROADCAST_VAL).toString());
}

void lmcUdpNetwork::start(void) {
	//	start receiving datagrams
	canReceive = startReceiving();
	isRunning = true;
}

void lmcUdpNetwork::stop(void) {
	disconnect(pUdpReceiver, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
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
	broadcastAddress = QHostAddress(pSettings->value(IDS_BROADCAST, IDS_BROADCAST_VAL).toString());
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

	pUdpSender->writeDatagram(datagram.data(), datagram.size(), remoteAddress, nUdpPort);
}

bool lmcUdpNetwork::startReceiving(void) {
	if(pUdpReceiver->bind(nUdpPort, QUdpSocket::ShareAddress)) {
		connect(pUdpReceiver, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
		return true;
	}
	return false;
}

void lmcUdpNetwork::parseDatagram(QString* lpszAddress, QByteArray& baDatagram) {
	DatagramHeader* pHeader = new DatagramHeader(DT_Broadcast, QString(), *lpszAddress);
	QString szData = QString::fromUtf8(baDatagram.data(), baDatagram.length());
	emit broadcastReceived(pHeader, &szData);
}
