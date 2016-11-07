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


#ifndef UDPNETWORK_H
#define UDPNETWORK_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkAddressEntry>
#include <QHostAddress>
#include "shared.h"
#include "datagram.h"
#include "settings.h"
#include "crypto.h"
#include "xmlmessage.h"

class lmcUdpNetwork : public QObject {
	Q_OBJECT

public:
	lmcUdpNetwork(void);
	~lmcUdpNetwork(void);

	void init(void);
	void start(void);
	void stop(void);
	void setLocalId(QString* lpszLocalId);
	void setCrypto(lmcCrypto* pCrypto);
	void sendBroadcast(QString* lpszData);
	void settingsChanged(void);
	void setMulticastInterface(const QNetworkInterface& networkInterface);
	void setListenAddress(const QString& szAddress);

	QString ipAddress;
	QString subnetMask;
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

	lmcSettings*		pSettings;
	QUdpSocket*			pUdpSocket;
	lmcCrypto*			pCrypto;

	bool				isRunning;
	int					nUdpPort;
	QHostAddress		broadcastAddress;
	QString				localId;
	QNetworkInterface	multicastInterface;
	QHostAddress		listenAddress;
};

#endif // UDPNETWORK_H
