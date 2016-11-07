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


#include "messaging.h"

lmcMessaging::lmcMessaging(void) {
	pNetwork = new lmcNetwork();
	connect(pNetwork, SIGNAL(broadcastReceived(DatagramHeader*, QString*)), 
		this, SLOT(receiveBroadcast(DatagramHeader*, QString*)));
	connect(pNetwork, SIGNAL(messageReceived(DatagramHeader*, QString*)), 
		this, SLOT(receiveMessage(DatagramHeader*, QString*)));
	connect(pNetwork, SIGNAL(newConnection(QString*, QString*)), 
		this, SLOT(newConnection(QString*, QString*)));
	connect(pNetwork, SIGNAL(connectionLost(QString*)),
		this, SLOT(connectionLost(QString*)));
	connect(pNetwork, SIGNAL(progressReceived(QString*, QString*)),
		this, SLOT(receiveProgress(QString*, QString*)));
	connect(pNetwork, SIGNAL(connectionStateChanged()), this, SLOT(network_connectionStateChanged()));
	userList.clear();
	groupList.clear();
	groupMap.clear();
	receivedList.clear();
	pendingList.clear();
	loopback = false;
}

lmcMessaging::~lmcMessaging(void) {
}

void lmcMessaging::init(void) {
	pNetwork->init();

	QString logonName = Helper::getLogonName();
	QString szAddress = pNetwork->getPhysicalAddress();
	QString userId = createUserId(&szAddress, &logonName);

	pNetwork->setLocalId(&userId);
	
	pSettings = new lmcSettings();
	QString userStatus = pSettings->value(IDS_STATUS, IDS_STATUS_VAL).toString();
	int sIndex = Helper::statusIndexFromCode(userStatus);
	//	if status not recognized, default to available
	if(sIndex < 0)
		userStatus = statusCode[0];
	QString userName = getUserName();

	int nAvatar = pSettings->value(IDS_AVATAR, IDS_AVATAR_VAL).toInt();
	localUser = new User(userId, IDA_VERSION, pNetwork->ipAddress, userName, userStatus, QString::null, nAvatar);

	int size = pSettings->beginReadArray(IDS_GROUPHDR);
	for(int index = 0; index < size; index++) {
		pSettings->setArrayIndex(index);
		groupList.append(pSettings->value(IDS_GROUP).toString());
	}
	pSettings->endArray();

	if(groupList.count() == 0)
		groupList.append(GRP_DEFAULT);

	size = pSettings->beginReadArray(IDS_GROUPMAPHDR);
	for(int index = 0; index < size; index++)
	{
		pSettings->setArrayIndex(index);
		QString user = pSettings->value(IDS_USER).toString();
		QString group = pSettings->value(IDS_GROUP).toString();
		groupMap.insert(user, group);
	}
	pSettings->endArray();

	nTimeout = pSettings->value(IDS_TIMEOUT, IDS_TIMEOUT_VAL).toInt() * 1000;
	nMaxRetry = pSettings->value(IDS_MAXRETRIES, IDS_MAXRETRIES_VAL).toInt();

	pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
	pTimer->start(1000);

	msgId = 1;
}

void lmcMessaging::start(void) {
	pNetwork->start();

	sendBroadcast(MT_Depart, NULL);
	sendBroadcast(MT_Announce, NULL);
}

void lmcMessaging::update(void) {
	sendBroadcast(MT_Announce, NULL);

	for(int index = 0; index < userList.count(); index++)
		sendMessage(MT_Ping, &userList[index].id, NULL);
}

void lmcMessaging::stop(void) {
    sendBroadcast(MT_Depart, NULL);
	pNetwork->stop();

	pSettings->setValue(IDS_STATUS, localUser->status);
	pSettings->setValue(IDS_AVATAR, localUser->avatar);

	pSettings->beginWriteArray(IDS_GROUPHDR);
	for(int index = 0; index < groupList.count(); index++) {
		pSettings->setArrayIndex(index);
		pSettings->setValue(IDS_GROUP, groupList[index]);
	}
	pSettings->endArray();

	pSettings->beginWriteArray(IDS_GROUPMAPHDR);
	QMapIterator<QString, QString> i(groupMap);
	int count = 0;
	while(i.hasNext()) {
		pSettings->setArrayIndex(count);
		i.next();
		pSettings->setValue(IDS_USER, i.key());
		pSettings->setValue(IDS_GROUP, i.value());
		count++;
	}
	pSettings->endArray();
}

bool lmcMessaging::isConnected(void) {
	return pNetwork->isConnected;
}

bool lmcMessaging::canReceive(void) {
	return pNetwork->canReceive;
}

void lmcMessaging::setLoopback(bool on) {
	loopback = on;
}

User* lmcMessaging::getUser(QString* id) {
	for(int index = 0; index < userList.count(); index++)
		if(userList[index].id.compare(*id) == 0)
			return &userList[index];

	return NULL;
}

void lmcMessaging::settingsChanged(void) {
	nTimeout = pSettings->value(IDS_TIMEOUT, IDS_TIMEOUT_VAL).toInt() * 1000;
	nMaxRetry = pSettings->value(IDS_MAXRETRIES, IDS_MAXRETRIES_VAL).toInt();
	pNetwork->settingsChanged();

	QString userName = getUserName();
	if(localUser->name.compare(userName) != 0) {
		localUser->name = userName;
		XmlMessage xmlMessage;
		xmlMessage.addData(XN_NAME, userName);
		sendMessage(MT_UserName, NULL, &xmlMessage);
	}
}

void lmcMessaging::network_connectionStateChanged(void) {
	if(isConnected())
		localUser->address = pNetwork->ipAddress;
	emit connectionStateChanged();
}

void lmcMessaging::timer_timeout(void) {
	//	check if any pending message has timed out
	checkPendingMsg();
}

QString lmcMessaging::createUserId(QString* lpszAddress, QString* lpszUserName) {
	QString userId = *lpszAddress;
	userId.append(lpszUserName);
	userId.remove(":");

	return userId;
}

QString lmcMessaging::getUserName(void) {
	QString userName = pSettings->value(IDS_USERNAME, IDS_USERNAME_VAL).toString();
	if(userName.isEmpty())
		userName = Helper::getLogonName();
	return userName;
}

void lmcMessaging::getUserInfo(XmlMessage* pMessage) {
	QString info;
	QString firstName = pSettings->value(IDS_USERFIRSTNAME, IDS_USERFIRSTNAME_VAL).toString();
	QString lastName = pSettings->value(IDS_USERLASTNAME, IDS_USERLASTNAME_VAL).toString();
	QString about = pSettings->value(IDS_USERABOUT, IDS_USERABOUT_VAL).toString();
	firstName = firstName.isEmpty() ? "N/A" : firstName;
	lastName = lastName.isEmpty() ? "N/A" : lastName;
	about = about.isEmpty() ? "N/A" : about;

	pMessage->addData(XN_USERID, localUser->id);
	pMessage->addData(XN_NAME, localUser->name);
	pMessage->addData(XN_ADDRESS, localUser->address);
	pMessage->addData(XN_VERSION, localUser->version);
	pMessage->addData(XN_STATUS, localUser->status);
	pMessage->addData(XN_LOGON, Helper::getLogonName());
	pMessage->addData(XN_HOST, Helper::getHostName());
	pMessage->addData(XN_OS, Helper::getOSName());
	pMessage->addData(XN_FIRSTNAME, firstName);
	pMessage->addData(XN_LASTNAME, lastName);
	pMessage->addData(XN_ABOUT, about);
}

bool lmcMessaging::addUser(QString szUserId, QString szVersion, QString szAddress, QString szName, QString szStatus, QString szAvatar) {
	for(int index = 0; index < userList.count(); index++)
		if(userList[index].id.compare(szUserId) == 0)
			return false;

	if(!groupMap.contains(szUserId))
		groupMap.insert(szUserId, groupList[0]);

	int nAvatar = szAvatar.isNull() ? -1 : szAvatar.toInt();

	userList.append(User(szUserId, szVersion, szAddress, szName, szStatus, groupMap[szUserId], nAvatar));
	if(!szStatus.isNull()) {
		XmlMessage xmlMessage;
		xmlMessage.addHeader(XN_FROM, szUserId);
		xmlMessage.addData(XN_STATUS, szStatus);
		//	send a status message to app layer, this is different from announce message
		emit messageReceived(MT_Status, &szUserId, &xmlMessage);
		int statusIndex = Helper::statusIndexFromCode(szStatus);
		if(statusType[statusIndex] == StatusTypeOffline) // offline status
			return false;	//	no need to send a new user message to app layer
	}

	emit messageReceived(MT_Announce, &szUserId, NULL);
	return true;
}

void lmcMessaging::updateUser(MessageType type, QString szUserId, QString szUserData) {
	User* pUser = getUser(&szUserId);
	if(!pUser)
		return;

	XmlMessage updateMsg;	
	switch(type) {
	case MT_Status:
		if(pUser->status.compare(szUserData) != 0) {
			int statusIndex = Helper::statusIndexFromCode(pUser->status);
			if(statusType[statusIndex] == StatusTypeOffline) // old status is offline
				emit messageReceived(MT_Announce, &szUserId, NULL);
				
			pUser->status = szUserData;
			updateMsg.addData(XN_STATUS, pUser->status);
			emit messageReceived(MT_Status, &szUserId, &updateMsg);

			statusIndex = Helper::statusIndexFromCode(pUser->status);
			if(statusType[statusIndex] == StatusTypeOffline) // new status is offline
				emit messageReceived(MT_Depart, &szUserId, NULL);
		}
		break;
	case MT_UserName:
		if(pUser->name.compare(szUserData) != 0) {
			pUser->name = szUserData;
			updateMsg.addData(XN_NAME, pUser->name);
			emit messageReceived(MT_UserName, &szUserId, &updateMsg);
		}
		break;
	case MT_Group:
		pUser->group = szUserData;
		groupMap.insert(pUser->id, pUser->group);
		break;
	default:
		break;
	}
}

void lmcMessaging::removeUser(QString szUserId) {
	for(int index = 0; index < userList.count(); index++)
		if(userList.value(index).id.compare(szUserId) == 0) {
			XmlMessage statusMsg;
			statusMsg.addData(XN_STATUS, statusCode[ST_COUNT - 1]);
			emit messageReceived(MT_Status, &szUserId, &statusMsg);
			emit messageReceived(MT_Depart, &szUserId, NULL);
			userList.removeAt(index);
			return;
		}
}

bool lmcMessaging::addReceivedMsg(qint64 msgId, QString userId) {
	ReceivedMsg received(msgId, userId);

	if(receivedList.contains(received))
		return false;

	receivedList.append(received);
	return true;
}

void lmcMessaging::addPendingMsg(qint64 msgId, MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	XmlMessage xmlMessage;
	if(pMessage)
		xmlMessage = pMessage->clone();
	pendingList.append(PendingMsg(msgId, true, QDateTime::currentDateTime(), type, *lpszUserId, xmlMessage, 0));
}

void lmcMessaging::removePendingMsg(qint64 msgId) {
	for(int index = 0; index < pendingList.count(); index++) {
		if(pendingList[index].msgId == msgId) {
			pendingList[index].active = false;
			pendingList.removeAt(index);
			return;
		}
	}
}

void lmcMessaging::removeAllPendingMsg(QString* lpszUserId) {
	for(int index = 0; index < pendingList.count(); index++) {
		if(pendingList[index].userId.compare(*lpszUserId) == 0) {
			pendingList.removeAt(index);
			index--;
		}
	}
}

void lmcMessaging::checkPendingMsg(void) {
	for(int index = 0; index < pendingList.count(); index++) {
		//	check if message has timed out
		if(pendingList[index].active && pendingList[index].timeStamp.msecsTo(QDateTime::currentDateTime()) > nTimeout) {
			if(pendingList[index].retry < nMaxRetry) {
				//	send the message once more
				pendingList[index].retry++;
				pendingList[index].timeStamp = QDateTime::currentDateTime();
				resendMessage(pendingList[index].type, pendingList[index].msgId, &pendingList[index].userId, &pendingList[index].xmlMessage);
			}
			else {
				XmlMessage statusMsg;
				//	max retries exceeded. mark message as failed.
				switch(pendingList[index].type) {
				case MT_Message:
					emit messageReceived(MT_Failed, &pendingList[index].userId, &pendingList[index].xmlMessage);
					break;
				case MT_Ping:
					statusMsg.addData(XN_STATUS, statusCode[ST_COUNT - 1]);
					emit messageReceived(MT_Status, &pendingList[index].userId, &statusMsg);
					emit messageReceived(MT_Depart, &pendingList[index].userId, NULL);
					removeUser(pendingList[index].userId);
					break;
                default:
                    break;
				}
				pendingList[index].active = false;
				pendingList.removeAt(index);
				index--;	//	since next item will have this index now
			}
		}
	}
}

void lmcMessaging::resendMessage(MessageType type, qint64 msgId, QString* lpszUserId, XmlMessage* pMessage) {
	if(lpszUserId && !getUser(lpszUserId))
		return;

	prepareMessage(type, msgId, true, lpszUserId, pMessage);
}
