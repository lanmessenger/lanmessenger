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


#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QtWidgets/QApplication>
#include <QDir>
#include "shared.h"

//	Application settings definitions and default values
#define IDS_VERSION				"Application/Version"
#define IDS_VERSION_VAL			"1.2.10"
#define IDS_OPENPATH			"Application/OpenPath"
#define IDS_OPENPATH_VAL		QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
#define IDS_SAVEPATH			"Application/SavePath"
#define IDS_SAVEPATH_VAL		QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
#define IDS_WINDOWMAIN			"Window/Main"
#define IDS_WINDOWTRANSFERS		"Window/Transfers"
#define IDS_WINDOWHISTORY		"Window/History"
#define IDS_WINDOWBROADCAST		"Window/Broadcast"
#define IDS_WINDOWHELP			"Window/Help"
#define IDS_WINDOWPUBLICCHAT	"Window/PublicChat"
#define IDS_SPLITTERHISTORY		"Splitter/History"
#define IDS_SPLITTERBROADCAST	"Splitter/Broadcast"
#define IDS_SPLITTERPUBLICCHATH	"Splitter/PublicChatH"
#define IDS_SPLITTERPUBLICCHATV	"Splitter/PublicChatV"
#define IDS_AUTOSTART			"AutoStart"
#define IDS_AUTOSTART_VAL		true
#define IDS_AUTOSHOW			"AutoShow"
#define IDS_AUTOSHOW_VAL		false
#define IDS_LANGUAGE			"Locale/Language"
#define IDS_LANGUAGE_VAL		"en_US"
#define IDS_SYSTRAY				"SystemTray/SysTray"
#define IDS_SYSTRAY_VAL			true
#define IDS_MINIMIZETRAY		"SystemTray/MinimizeTray"
#define IDS_MINIMIZETRAY_VAL	false
#define IDS_SINGLECLICKTRAY		"SystemTray/SingleClickTray"
#define IDS_SINGLECLICKTRAY_VAL	false
#define IDS_MINIMIZEMSG			"SystemTray/MinimizeMsg"
#define IDS_MINIMIZEMSG_VAL		true
#define IDS_SYSTRAYMSG			"SystemTray/SysTrayMsg"
#define IDS_SYSTRAYMSG_VAL		true
#define IDS_ALLOWSYSTRAYMIN		"SystemTray/AllowMinimize"
#define IDS_ALLOWSYSTRAYMIN_VAL	false
#define IDS_REFRESHTIME			"RefreshInterval"
#define IDS_REFRESHTIME_VAL		300
#define IDS_IDLETIME			"IdleTime"
#define IDS_IDLETIME_VAL		0
#define IDS_EMOTICON			"Messages/Emoticon"
#define IDS_EMOTICON_VAL		true
#define IDS_MESSAGETIME			"Messages/MessageTime"
#define IDS_MESSAGETIME_VAL		true
#define IDS_MESSAGEDATE			"Messages/MessageDate"
#define IDS_MESSAGEDATE_VAL		false
#define IDS_ALLOWLINKS			"Messages/AllowLinks"
#define IDS_ALLOWLINKS_VAL		true
#define IDS_PATHTOLINK			"Messages/PathToLink"
#define IDS_PATHTOLINK_VAL		false
#define IDS_TRIMMESSAGE			"Messages/Trim"
#define IDS_TRIMMESSAGE_VAL		true
#define IDS_CLEARONCLOSE        "Messages/ClearOnClose"
#define IDS_CLEARONCLOSE_VAL    true
#define IDS_MESSAGETOP			"Messages/MessageTop"
#define IDS_MESSAGETOP_VAL		false
#define IDS_PUBMESSAGEPOP		"Messages/PubMessagePop"
#define IDS_PUBMESSAGEPOP_VAL	false
#define IDS_FONT				"Messages/Font"
#define IDS_FONT_VAL			QApplication::font().toString()
#define IDS_COLOR				"Messages/Color"
#define IDS_COLOR_VAL			QApplication::palette().text().color().name()
#define IDS_FONTSIZE			"Messages/FontSize"
#define IDS_FONTSIZE_VAL		FS_MEDIUM
#define IDS_HISTORY				"History/History"
#define IDS_HISTORY_VAL			true
#define IDS_SYSHISTORYPATH		"History/SysHistoryPathDef"
#define IDS_SYSHISTORYPATH_VAL	true
#define IDS_HISTORYPATH			"History/HistoryPath"
#define IDS_HISTORYPATH_VAL		""
#define IDS_FILEHISTORY			"History/FileHistory"
#define IDS_FILEHISTORY_VAL		true
#define IDS_FILEHISTORYPATH		"History/FileHistoryPath"
#define IDS_FILEHISTORYPATH_VAL	""
#define IDS_ALERT				"Alerts/Alert"
#define IDS_ALERT_VAL			true
#define IDS_NOBUSYALERT			"Alerts/NoBusyAlert"
#define IDS_NOBUSYALERT_VAL		false
#define IDS_NODNDALERT			"Alerts/NoDNDAlert"
#define IDS_NODNDALERT_VAL		true
#define IDS_SOUND				"Alerts/Sound"
#define IDS_SOUND_VAL			true
#define IDS_NOBUSYSOUND			"Alerts/NoBusySound"
#define IDS_NOBUSYSOUND_VAL		false
#define IDS_NODNDSOUND			"Alerts/NoDNDSound"
#define IDS_NODNDSOUND_VAL		true
#define IDS_SOUNDEVENTHDR		"SoundEvents"
#define IDS_SOUNDEVENT			"Event"
#define IDS_SOUNDEVENT_VAL		Qt::Checked
#define IDS_SOUNDFILEHDR		"SoundFiles"
#define IDS_SOUNDFILE				"File"
#define IDS_CONNECTION			"Connection/Connection"
#define IDS_CONNECTION_VAL		AUTO_CONNECTION
#define IDS_TIMEOUT				"Connection/Timeout"
#define IDS_TIMEOUT_VAL			10
#define IDS_MAXRETRIES			"Connection/MaxRetries"
#define IDS_MAXRETRIES_VAL		1
#define IDS_BROADCAST_OLD		"Connection/Broadcast"
#define IDS_BROADCAST_OLD_VAL	"255.255.255.255"
#define IDS_MULTICAST			"Connection/Multicast"
#define IDS_MULTICAST_VAL		"239.255.100.100"
#define IDS_UDPPORT				"Connection/UDPPort"
#define IDS_UDPPORT_VAL			50000
#define IDS_TCPPORT				"Connection/TCPPort"
#define IDS_TCPPORT_VAL			50000
#define IDS_AUTOFILE			"FileTransfer/AutoFile"
#define IDS_AUTOFILE_VAL		false
#define	IDS_AUTOSHOWFILE		"FileTransfer/AutoShow"
#define	IDS_AUTOSHOWFILE_VAL	true
#define IDS_FILETOP				"FileTransfer/FileTop"
#define IDS_FILETOP_VAL			false
#define IDS_FILESTORAGEPATH		"FileTransfer/StoragePath"
#define IDS_FILESTORAGEPATH_VAL	""
#define IDS_THEME_OLD			"Themes/Theme"
#define IDS_THEME_OLD_VAL		":/themes/Classic"
#define IDS_THEME				"Appearance/Theme"
#define IDS_THEME_VAL			":/themes/Classic"
#define IDS_USERLISTVIEW		"Appearance/UserListView"
#define IDS_USERLISTVIEW_VAL	ULV_Detailed
#define IDS_STATUSTOOLTIP		"Appearance/StatusToolTip"
#define IDS_STATUSTOOLTIP_VAL	false
#define IDS_SENDKEYMOD			"Hotkeys/SendKeyMod"
#define IDS_SENDKEYMOD_VAL		false
#define IDS_STATUS				"User/Status"
#define IDS_STATUS_VAL			statusCode[0]
#define IDS_AVATAR				"User/Avatar"
#define IDS_AVATAR_VAL			65535	//	this should be a number bigger than AVT_COUNT, 65535 set arbitrarily
#define IDS_USERNAME			"User/Name"
#define IDS_USERNAME_VAL		""
#define IDS_USERFIRSTNAME		"User/FirstName"
#define IDS_USERFIRSTNAME_VAL	""
#define IDS_USERLASTNAME		"User/LastName"
#define IDS_USERLASTNAME_VAL	""
#define IDS_USERABOUT			"User/About"
#define IDS_USERABOUT_VAL		""
#define IDS_NOTE				"User/Note"
#define IDS_NOTE_VAL			""
#define IDS_GROUPHDR			"Groups"
#define IDS_GROUP				"Group"
#define IDS_GROUPNAME			"GroupName"
#define IDS_GROUPEXPHDR			"GroupExp"
#define IDS_GROUPMAPHDR			"GroupMap"
#define IDS_USER				"User"
#define IDS_BROADCASTHDR		"BroadcastList"
#define IDS_BROADCAST			"Broadcast"

class lmcSettingsBase : public QSettings {
public:
	lmcSettingsBase(void);
	lmcSettingsBase(const QString& fileName, Format format);
	lmcSettingsBase(Format format, Scope scope, const QString& organization, const QString& application);
	~lmcSettingsBase(void);

	using QSettings::setValue;
	void setValue(const QString& key, const QVariant& value, const QVariant& defaultValue);
};

class lmcSettings : public lmcSettingsBase {
public:
	lmcSettings(void) : lmcSettingsBase(QSettings::IniFormat, QSettings::UserScope, IDA_COMPANY, IDA_PRODUCT) {}
	~lmcSettings(void) {}

	bool migrateSettings(void);
	bool loadFromConfig(const QString& configFile);

    static void setAutoStart(bool on);

private:
	bool migrateSettings(const QString& configFile);
};

#endif // SETTINGS_H
