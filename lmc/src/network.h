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


#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QNetworkAddressEntry>
#include <QHostAddress>
#include <QTimer>
#include "udpnetwork.h"
#include "tcpnetwork.h"
#include "webnetwork.h"
#include "crypto.h"
#include "settings.h"
#include "xmlmessage.h"

class lmcNetwork : public QObject {
	Q_OBJECT

public:
	lmcNetwork(void);
	~lmcNetwork(void);

	void init(XmlMessage* pInitParams);
	void start(void);
	void stop(void);
	QString physicalAddress(void);
	void setLocalId(QString* lpszLocalId);
	void sendBroadcast(QString* lpszData);
	void addConnection(QString* lpszUserId, QString* lpszAddress);
	void sendMessage(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData);
	void initSendFile(QString* lpszReceiverId, QString* lpszAddress, QString* lpszData);
	void initReceiveFile(QString* lpszSenderId, QString* lpszAddress, QString* lpszData);
	void fileOperation(FileMode mode, QString* lpszUserId, QString* lpszData);
	void sendWebMessage(QString* lpszUrl, QString* lpszData);
	void settingsChanged(void);

	QString	ipAddress;
	QString	subnetMask;
	bool	isConnected;
	bool	canReceive;

signals:
	void connectionStateChanged(void);
	void broadcastReceived(DatagramHeader* pHeader, QString* lpszData);
	void newConnection(QString* lpszUserId, QString *lpszAddress);
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
    bool getIPAddress(bool verbose = true);
	bool getIPAddress(QNetworkInterface* pNetworkInterface, QNetworkAddressEntry* pAddressEntry);
	bool getNetworkInterface(QNetworkInterface* pNetworkInterface);
	bool getNetworkInterface(QNetworkInterface* pNetworkInterface, QString* lpszPreferred);
	bool isInterfaceUp(QNetworkInterface* pNetworkInterface);
	bool getNetworkAddressEntry(QNetworkAddressEntry* pAddressEntry);

	struct NetworkAdapter {
		QString name;
		QString description;
		QString type;
	};

	lmcSettings*			pSettings;
	lmcUdpNetwork*			pUdpNetwork;
	lmcTcpNetwork*			pTcpNetwork;
	lmcWebNetwork*			pWebNetwork;
	lmcCrypto*				pCrypto;
	QTimer*					pTimer;
    QString					interfaceName;
	QNetworkInterface		networkInterface;
};

#endif // NETWORK_H
