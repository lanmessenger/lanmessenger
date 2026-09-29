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


#include "lmcstrings.h"

QString lmcStrings::m_appName;
QString lmcStrings::m_appDesc;
QString lmcStrings::m_autoConn;
QStringList lmcStrings::m_fontSize;
QStringList lmcStrings::m_statusDesc;
QStringList lmcStrings::m_soundDesc;
QStringList lmcStrings::m_awayTimeDesc;
QStringList lmcStrings::m_userListView;

lmcStrings::lmcStrings(void) {
}

lmcStrings::~lmcStrings(void) {
}

void lmcStrings::retranslate(void) {
	m_appName.clear();
	m_appDesc.clear();
	m_autoConn.clear();
	m_fontSize.clear();
	m_statusDesc.clear();
	m_soundDesc.clear();
	m_awayTimeDesc.clear();
	m_userListView.clear();
}

const QString lmcStrings::appName(void) {
	if(m_appName.isEmpty())
		m_appName = tr("LAN Messenger");
	return m_appName;
}

const QString lmcStrings::appDesc(void) {
	if(m_appDesc.isEmpty())
        m_appDesc = tr("LAN Messenger is a free peer-to-peer messaging application for\n"\
                       "intra-network communication and does not require a server.\n"\
                       "LAN Messenger works on essentially every popular desktop platform.");
	return m_appDesc;
}

const QString lmcStrings::autoConn(void) {
	if(m_autoConn.isEmpty())
		m_autoConn = tr("Automatic");
	return m_autoConn;
}

const QStringList lmcStrings::fontSize(void) {
	if(m_fontSize.isEmpty()) {
		m_fontSize.append(tr("Small text"));
		m_fontSize.append(tr("Medium text"));
		m_fontSize.append(tr("Large text"));
	}
	return m_fontSize;
}

const QStringList lmcStrings::statusDesc(void) {
	if(m_statusDesc.isEmpty()) {
		m_statusDesc.append(tr("Available"));
		m_statusDesc.append(tr("Busy"));
		m_statusDesc.append(tr("Do Not Disturb"));
		m_statusDesc.append(tr("Be Right Back"));
		m_statusDesc.append(tr("Away"));
		m_statusDesc.append(tr("Appear Offline"));
	}
	return m_statusDesc;
}

const QStringList lmcStrings::soundDesc(void) {
	if(m_soundDesc.isEmpty()) {
		m_soundDesc.append(tr("Incoming message"));
		m_soundDesc.append(tr("User is online"));
		m_soundDesc.append(tr("User is offline"));
		m_soundDesc.append(tr("Incoming file transfer"));
		m_soundDesc.append(tr("File transfer completed"));
		m_soundDesc.append(tr("Incoming public message"));
	}
	return m_soundDesc;
}

const QStringList lmcStrings::awayTimeDesc(void) {
	if(m_awayTimeDesc.isEmpty()) {
		m_awayTimeDesc.append(tr("5 minutes"));
		m_awayTimeDesc.append(tr("10 minutes"));
		m_awayTimeDesc.append(tr("15 minutes"));
		m_awayTimeDesc.append(tr("20 minutes"));
		m_awayTimeDesc.append(tr("30 minutes"));
		m_awayTimeDesc.append(tr("45 minutes"));
		m_awayTimeDesc.append(tr("60 minutes"));
		m_awayTimeDesc.append(tr("Never"));
	}
	return m_awayTimeDesc;
}

const QStringList lmcStrings::userListView(void) {
	if(m_userListView.isEmpty()) {
		m_userListView.append(tr("Detailed"));
		m_userListView.append(tr("Compact"));
	}
	return m_userListView;
}
