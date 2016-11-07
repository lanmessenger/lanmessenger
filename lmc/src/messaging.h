/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
** 
** Contact:  dilipvradhakrishnan@gmail.com
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


#ifndef MESSAGING_H
#define MESSAGING_H

#include <QObject>
#include <QTimer>
#include <QFileInfo>
#include "shared.h"
#include "message.h"
#include "network.h"
#include "settings.h"

class lmcMessaging : public QObject {
	Q_OBJECT

public:
	lmcMessaging(void);
	~lmcMessaging(void);

	void init(void);
	void start(void);
	void update(void);
	void stop(void);
	bool isConnected(void);
	bool canReceive(void);
	void setLoopback(bool on);
	User* getUser(QString* lpszUserId);
	void sendBroadcast(MessageType type, QString* lpszData);
	void sendMessage(MessageType type, QString* lpszUserId, QString* lpszData);
	void settingsChanged(void);

	User* localUser;
	QList<User> userList;
	QList<QString> groupList;
	QMap<QString, QString> groupMap;

signals:
	void messageReceived(MessageType type, QString* lpszUserId, QString* lpszData);
	void connectionStateChanged(void);

private slots:
	void receiveBroadcast(DatagramHeader* pHeader, QString* lpszData);
	void receiveMessage(DatagramHeader* pHeader, QString* lpszData);
	void newConnection(QString* lpszUserId, QString* lpszAddress);
	void connectionLost(QString* lpszUserId);
	void receiveProgress(QString* lpszUserId, QString* lpszData);
	void network_connectionStateChanged(void);
	void timer_timeout(void);

private:
	QString createUserId(QString* lpszAddress, QString* lpszUserName);
	QString getUserName(void);
	QString getUserInfo(void);
	void sendUserData(MessageType type, QString* lpszUserId, QString* lpszAddress);
	void prepareBroadcast(MessageType type, QString* lpszData);
	void prepareMessage(MessageType type, qint64 msgId, bool retry, QString* lpszUserId, QString* lpszData);
	void prepareFile(MessageType type, qint64 msgId, bool retry, QString* lpszUserId, QString* lpszData);
	void processBroadcast(MessageHeader* pHeader, QString* lpszData);
	void processMessage(MessageHeader* pHeader, QString* lpszData);
	void processFile(MessageHeader* pHeader, QString* lpszData);
	bool addUser(QString szUserId, QString szVersion, QString szAddress, QString szName, QString szStatus, QString szAvatar);
	void updateUser(QString szUserId, QString szUserData);
	void removeUser(QString szUserId);
	bool addReceivedMsg(qint64 msgId, QString userId);
	void addPendingMsg(qint64 msgId, MessageType type, QString userId, QString data);
	void removePendingMsg(qint64);
	void removeAllPendingMsg(QString* lpszUserId);
	void checkPendingMsg(void);
	void resendMessage(MessageType type, qint64 msgId, QString* lpszUserId, QString* lpszData);

	lmcNetwork*		pNetwork;
	lmcSettings*	pSettings;
	QTimer*			pTimer;
	qint64			msgId;
	QList<ReceivedMsg> receivedList;
	QList<PendingMsg> pendingList;
	int				nTimeout;
	int				nMaxRetry;
	bool			loopback;
};

#endif // MESSAGING_H