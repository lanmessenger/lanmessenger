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


#include "messaging.h"
#include "stdlocation.h"
#include "trace.h"

lmcMessaging::lmcMessaging(void) {
	pNetwork = new lmcNetwork();
	connect(pNetwork, SIGNAL(broadcastReceived(DatagramHeader*, QString*)), 
		this, SLOT(receiveBroadcast(DatagramHeader*, QString*)));
	connect(pNetwork, SIGNAL(messageReceived(DatagramHeader*, QString*)), 
		this, SLOT(receiveMessage(DatagramHeader*, QString*)));
	connect(pNetwork, SIGNAL(webMessageReceived(QString*)),
		this, SLOT(receiveWebMessage(QString*)));
	connect(pNetwork, SIGNAL(newConnection(QString*, QString*)), 
		this, SLOT(newConnection(QString*, QString*)));
	connect(pNetwork, SIGNAL(connectionLost(QString*)),
		this, SLOT(connectionLost(QString*)));
	connect(pNetwork, SIGNAL(progressReceived(QString*, QString*)),
		this, SLOT(receiveProgress(QString*, QString*)));
	connect(pNetwork, SIGNAL(connectionStateChanged()), this, SLOT(network_connectionStateChanged()));
    localUser = NULL;
	userList.clear();
	groupList.clear();
	userGroupMap.clear();
	receivedList.clear();
	pendingList.clear();
    fileList.clear();
    folderList.clear();
	loopback = false;
}

lmcMessaging::~lmcMessaging(void) {
}

void lmcMessaging::init(XmlMessage *pInitParams) {
	lmcTrace::write("Messaging initialized");

	pNetwork->init(pInitParams);

    QString logonName = Helper::getLogonName();
    QString szAddress = pNetwork->physicalAddress();
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
	QString userNote = pSettings->value(IDS_NOTE, IDS_NOTE_VAL).toString();
    uint userCaps = UC_File | UC_GroupMessage | UC_Folder;
    localUser = new User(userId, IDA_VERSION, pNetwork->ipAddress, userName, userStatus,
                         QString(), nAvatar, userNote, StdLocation::avatarFile(),
                         QString::number(userCaps));

	loadGroups();

	nTimeout = pSettings->value(IDS_TIMEOUT, IDS_TIMEOUT_VAL).toInt() * 1000;
	nMaxRetry = pSettings->value(IDS_MAXRETRIES, IDS_MAXRETRIES_VAL).toInt();

	pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
	pTimer->start(1000);

	msgId = 1;
}

void lmcMessaging::start(void) {
	lmcTrace::write("Messaging started");
	pNetwork->start();

	sendBroadcast(MT_Depart, NULL);
	sendBroadcast(MT_Announce, NULL);
}

void lmcMessaging::update(void) {
	lmcTrace::write("Refreshing contacts list...");
	sendBroadcast(MT_Announce, NULL);

	for(int index = 0; index < userList.count(); index++)
		sendMessage(MT_Ping, &userList[index].id, NULL);
}

void lmcMessaging::stop(void) {
    sendBroadcast(MT_Depart, NULL);
	pNetwork->stop();

	pSettings->setValue(IDS_STATUS, localUser->status);
	pSettings->setValue(IDS_AVATAR, localUser->avatar);

	saveGroups();

	lmcTrace::write("Messaging stopped");
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

void lmcMessaging::updateGroup(GroupOp op, QVariant value1, QVariant value2) {
	switch(op) {
	case GO_New:
		groupList.append(Group(value1.toString(), value2.toString()));
		break;
	case GO_Rename:
		for(int index = 0; index < groupList.count(); index++) {
			if(groupList[index].id.compare(value1.toString()) == 0) {
				groupList[index].name = value2.toString();
				break;
			}
		}
		break;
	case GO_Move:
		for(int index = 0; index < groupList.count(); index++) {
			if(groupList[index].id.compare(value1.toString()) == 0) {
				groupList.move(index, value2.toInt());
				break;
			}
		}
		break;
	case GO_Delete:
		for(int index = 0; index < groupList.count(); index++) {
			if(groupList[index].id.compare(value1.toString()) == 0) {
				groupList.removeAt(index);
				break;
			}
		}
		break;
	default:
		break;
	}

	saveGroups();
}

void lmcMessaging::updateGroupMap(QString oldGroup, QString newGroup) {
	QMap<QString, QString>::const_iterator index = userGroupMap.constBegin();
	while (index != userGroupMap.constEnd()) {
		if(((QString)index.value()).compare(oldGroup) == 0)
			userGroupMap.insert(index.key(), newGroup);
		++index;
	}
}

//	save groups and group mapping
void lmcMessaging::saveGroups(void) {
	QSettings groupSettings(StdLocation::groupFile(), QSettings::IniFormat);
	groupSettings.beginWriteArray(IDS_GROUPHDR);
	for(int index = 0; index < groupList.count(); index++) {
		groupSettings.setArrayIndex(index);
		groupSettings.setValue(IDS_GROUP, groupList[index].id);
		groupSettings.setValue(IDS_GROUPNAME, groupList[index].name);
	}
	groupSettings.endArray();

	groupSettings.beginWriteArray(IDS_GROUPMAPHDR);
	QMapIterator<QString, QString> i(userGroupMap);
	int count = 0;
	while(i.hasNext()) {
		groupSettings.setArrayIndex(count);
		i.next();
		groupSettings.setValue(IDS_USER, i.key());
		groupSettings.setValue(IDS_GROUP, i.value());
		count++;
	}
	groupSettings.endArray();

	groupSettings.sync();

	// make sure the correct version is set in the preferences file
	// so the group settings will not be wrongly migrated next time
	// application starts
	pSettings->setValue(IDS_VERSION, IDA_VERSION);
}

int lmcMessaging::userCount(void) {
	return userList.count();
}

void lmcMessaging::network_connectionStateChanged(void) {
    if(isConnected()) {
		localUser->address = pNetwork->ipAddress;
        if(localUser->id.isNull()) {
            QString logonName = Helper::getLogonName();
            QString szAddress = pNetwork->physicalAddress();
            QString userId = createUserId(&szAddress, &logonName);
            localUser->id = userId;
            pNetwork->setLocalId(&userId);
        }
    }
	emit connectionStateChanged();
}

void lmcMessaging::timer_timeout(void) {
	//	check if any pending message has timed out
	checkPendingMsg();
}

QString lmcMessaging::createUserId(QString* lpszAddress, QString* lpszUserName) {
	QString userId = *lpszAddress;
    if(!userId.isNull()) {
        userId.append(lpszUserName);
        userId.remove(":");
    }
	return userId;
}

QString lmcMessaging::getUserName(void) {
	QString userName = pSettings->value(IDS_USERNAME, IDS_USERNAME_VAL).toString();
	if(userName.isEmpty())
		userName = Helper::getLogonName();
	return userName;
}

void lmcMessaging::loadGroups(void) {
	bool defaultFound = false;

	QSettings groupSettings(StdLocation::groupFile(), QSettings::IniFormat);
	int size = groupSettings.beginReadArray(IDS_GROUPHDR);
	for(int index = 0; index < size; index++) {
		groupSettings.setArrayIndex(index);
		QString groupId = groupSettings.value(IDS_GROUP).toString();
		QString group = groupSettings.value(IDS_GROUPNAME).toString();
		groupList.append(Group(groupId, group));
		// check if the default group is present in the group list
		if(groupId.compare(GRP_DEFAULT_ID) == 0)
			defaultFound = true;
	}
	groupSettings.endArray();

	if(groupList.count() == 0 || !defaultFound)
		groupList.append(Group(GRP_DEFAULT_ID, GRP_DEFAULT));

	size = groupSettings.beginReadArray(IDS_GROUPMAPHDR);
	for(int index = 0; index < size; index++)
	{
		groupSettings.setArrayIndex(index);
		QString user = groupSettings.value(IDS_USER).toString();
		QString group = groupSettings.value(IDS_GROUP).toString();
		userGroupMap.insert(user, group);
	}
	groupSettings.endArray();
}

void lmcMessaging::getUserInfo(XmlMessage* pMessage) {
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
	pMessage->addData(XN_NOTE, localUser->note);
	pMessage->addData(XN_LOGON, Helper::getLogonName());
	pMessage->addData(XN_HOST, Helper::getHostName());
	pMessage->addData(XN_OS, Helper::getOSName());
	pMessage->addData(XN_FIRSTNAME, firstName);
	pMessage->addData(XN_LASTNAME, lastName);
	pMessage->addData(XN_ABOUT, about);
}

bool lmcMessaging::addUser(QString szUserId, QString szVersion, QString szAddress, QString szName, QString szStatus,
                           QString szAvatar, QString szNote, QString szCaps) {
	for(int index = 0; index < userList.count(); index++)
		if(userList[index].id.compare(szUserId) == 0)
			return false;

	lmcTrace::write("Adding new user: " + szUserId + ", " + szVersion + ", " + szAddress);

	if(!userGroupMap.contains(szUserId) || !groupList.contains(Group(userGroupMap.value(szUserId))))
		userGroupMap.insert(szUserId, GRP_DEFAULT_ID);

	int nAvatar = szAvatar.isNull() ? -1 : szAvatar.toInt();

    userList.append(User(szUserId, szVersion, szAddress, szName, szStatus, userGroupMap[szUserId],
                         nAvatar, szNote, QString(), szCaps));
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
			QString oldStatus = pUser->status;
			pUser->status = szUserData;

			int statusIndex = Helper::statusIndexFromCode(oldStatus);
			if(statusType[statusIndex] == StatusTypeOffline) // old status is offline
				emit messageReceived(MT_Announce, &szUserId, NULL);
				
			updateMsg.addData(XN_STATUS, pUser->status);
			emit messageReceived(MT_Status, &szUserId, &updateMsg);

			statusIndex = Helper::statusIndexFromCode(pUser->status);
			if(statusType[statusIndex] == StatusTypeOffline) { // new status is offline
				// Send a dummy xml message. A non null xml message implies that the
				// user is only in offline status, and not actually offline.
				XmlMessage xmlMessage;
				emit messageReceived(MT_Depart, &szUserId, &xmlMessage);
			}
		}
		break;
	case MT_UserName:
		if(pUser->name.compare(szUserData) != 0) {
			pUser->name = szUserData;
			updateMsg.addData(XN_NAME, pUser->name);
			emit messageReceived(MT_UserName, &szUserId, &updateMsg);
		}
		break;
	case MT_Note:
		if(pUser->note.compare(szUserData) != 0) {
			pUser->note = szUserData;
			updateMsg.addData(XN_NOTE, pUser->note);
			emit messageReceived(MT_Note, &szUserId, &updateMsg);
		}
		break;
	case MT_Group:
		pUser->group = szUserData;
		userGroupMap.insert(pUser->id, pUser->group);
		saveGroups();
		break;
    case MT_Avatar:
        pUser->avatarPath = szUserData;
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
