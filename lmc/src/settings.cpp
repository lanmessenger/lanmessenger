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


#include "settings.h"
#include "stdlocation.h"

lmcSettingsBase::lmcSettingsBase(void) : QSettings() {
}

lmcSettingsBase::lmcSettingsBase(const QString& fileName, Format format) :
	QSettings(fileName, format) {
}

lmcSettingsBase::lmcSettingsBase(Format format, Scope scope, const QString& organization, const QString& application) :
	QSettings(format, scope, organization, application) {
}

lmcSettingsBase::~lmcSettingsBase(void) {
}

void lmcSettingsBase::setValue(const QString& key, const QVariant& value, const QVariant& defaultValue) {
    if(value != defaultValue)
        QSettings::setValue(key, value);
    else
        remove(key);
}

//	migrate settings from older versions to new format
//	Returns false if existing settings cannot be migrated, else true
bool lmcSettings::migrateSettings(void) {
	//	Make sure any pending write operation is completed
	sync();
	//	If settings file does not exist, return true indicating no error
	if(!QFile::exists(fileName()))
		return true;

	//	Migrate the settings
	bool migrated = migrateSettings(fileName());
	//	Sync the settings with the settings file
	sync();
	return migrated;
}

//	Load settings from the specified config file and overwrite corresponding
//	application settings
bool lmcSettings::loadFromConfig(const QString& configFile) {
	if(!QFile::exists(configFile))
		return false;

	if(!Helper::copyFile(configFile, StdLocation::tempConfigFile()))
		return false;

	if(!migrateSettings(StdLocation::tempConfigFile()))
		return false;

	QVariant value;
	QSettings extSettings(StdLocation::tempConfigFile(), QSettings::IniFormat);

	value = extSettings.value(IDS_AUTOSTART);
	if(value.isValid())	setValue(IDS_AUTOSTART, value);
	value = extSettings.value(IDS_AUTOSHOW);
	if(value.isValid())	setValue(IDS_AUTOSHOW, value);

	value = extSettings.value(IDS_SYSTRAY);
	if(value.isValid())	setValue(IDS_SYSTRAY, value);
	value = extSettings.value(IDS_MINIMIZETRAY);
	if(value.isValid())	setValue(IDS_MINIMIZETRAY, value);
	value = extSettings.value(IDS_SINGLECLICKTRAY);
	if(value.isValid())	setValue(IDS_SINGLECLICKTRAY, value);
	value = extSettings.value(IDS_SYSTRAYMSG);
	if(value.isValid())	setValue(IDS_SYSTRAYMSG, value);
	value = extSettings.value(IDS_ALLOWSYSTRAYMIN);
	if(value.isValid())	setValue(IDS_ALLOWSYSTRAYMIN, value);
	value = extSettings.value(IDS_LANGUAGE);
	if(value.isValid())	setValue(IDS_LANGUAGE, value);

	value = extSettings.value(IDS_USERNAME);
	if(value.isValid())	setValue(IDS_USERNAME, value);
	value = extSettings.value(IDS_USERFIRSTNAME);
	if(value.isValid())	setValue(IDS_USERFIRSTNAME, value);
	value = extSettings.value(IDS_USERLASTNAME);
	if(value.isValid())	setValue(IDS_USERLASTNAME, value);
	value = extSettings.value(IDS_USERABOUT);
	if(value.isValid())	setValue(IDS_USERABOUT, value);
	value = extSettings.value(IDS_REFRESHTIME);
	if(value.isValid())	setValue(IDS_REFRESHTIME, value);

	value = extSettings.value(IDS_MESSAGETOP);
	if(value.isValid())	setValue(IDS_MESSAGETOP, value);
	value = extSettings.value(IDS_PUBMESSAGEPOP);
	if(value.isValid())	setValue(IDS_PUBMESSAGEPOP, value);
	value = extSettings.value(IDS_EMOTICON);
	if(value.isValid())	setValue(IDS_EMOTICON, value);
	value = extSettings.value(IDS_MESSAGETIME);
	if(value.isValid())	setValue(IDS_MESSAGETIME, value);
	value = extSettings.value(IDS_MESSAGEDATE);
	if(value.isValid())	setValue(IDS_MESSAGEDATE, value);
	value = extSettings.value(IDS_ALLOWLINKS);
	if(value.isValid())	setValue(IDS_ALLOWLINKS, value);
	value = extSettings.value(IDS_PATHTOLINK);
	if(value.isValid())	setValue(IDS_PATHTOLINK, value);
	value = extSettings.value(IDS_TRIMMESSAGE);
	if(value.isValid())	setValue(IDS_TRIMMESSAGE, value);
	value = extSettings.value(IDS_FONT);
	if(value.isValid())	setValue(IDS_FONT, value);
	value = extSettings.value(IDS_COLOR);
	if(value.isValid())	setValue(IDS_COLOR, value);
	value = extSettings.value(IDS_FONTSIZE);
	if(value.isValid())	setValue(IDS_FONTSIZE, value);

	value = extSettings.value(IDS_HISTORY);
	if(value.isValid())	setValue(IDS_HISTORY, value);
	value = extSettings.value(IDS_SYSHISTORYPATH);
	if(value.isValid())	setValue(IDS_SYSHISTORYPATH, value);
	value = extSettings.value(IDS_FILEHISTORY);
	if(value.isValid())	setValue(IDS_FILEHISTORY, value);

	value = extSettings.value(IDS_ALERT);
	if(value.isValid())	setValue(IDS_ALERT, value);
	value = extSettings.value(IDS_NOBUSYALERT);
	if(value.isValid())	setValue(IDS_NOBUSYALERT, value);
	value = extSettings.value(IDS_NODNDALERT);
	if(value.isValid())	setValue(IDS_NODNDALERT, value);
	value = extSettings.value(IDS_SOUND);
	if(value.isValid())	setValue(IDS_SOUND, value);
	value = extSettings.value(IDS_NOBUSYSOUND);
	if(value.isValid())	setValue(IDS_NOBUSYSOUND, value);
	value = extSettings.value(IDS_NODNDSOUND);
	if(value.isValid())	setValue(IDS_NODNDSOUND, value);

	value = extSettings.value(IDS_TIMEOUT);
	if(value.isValid())	setValue(IDS_TIMEOUT, value);
	value = extSettings.value(IDS_MAXRETRIES);
	if(value.isValid())	setValue(IDS_MAXRETRIES, value);
	QStringList broadcastList;
	int size = extSettings.beginReadArray(IDS_BROADCASTHDR);
	for(int index = 0; index < size; index++) {
		extSettings.setArrayIndex(index);
		broadcastList.append(extSettings.value(IDS_BROADCAST).toString());
	}
	extSettings.endArray();
	if(size > 0) {
		beginWriteArray(IDS_BROADCASTHDR);
		for(int index = 0; index < size; index++) {
			setArrayIndex(index);
			setValue(IDS_BROADCAST, broadcastList.at(index));
		}
		endArray();
	}
	value = extSettings.value(IDS_MULTICAST);
	if(value.isValid())	setValue(IDS_MULTICAST, value);
	value = extSettings.value(IDS_UDPPORT);
	if(value.isValid())	setValue(IDS_UDPPORT, value);
	value = extSettings.value(IDS_TCPPORT);
	if(value.isValid())	setValue(IDS_TCPPORT, value);

	value = extSettings.value(IDS_AUTOFILE);
	if(value.isValid())	setValue(IDS_AUTOFILE, value);
	value = extSettings.value(IDS_AUTOSHOWFILE);
	if(value.isValid())	setValue(IDS_AUTOSHOWFILE, value);
	value = extSettings.value(IDS_FILETOP);
	if(value.isValid())	setValue(IDS_FILETOP, value);

	value = extSettings.value(IDS_THEME);
	if(value.isValid())	setValue(IDS_THEME, value);
	value = extSettings.value(IDS_USERLISTVIEW);
	if(value.isValid())	setValue(IDS_USERLISTVIEW, value);
	value = extSettings.value(IDS_STATUSTOOLTIP);
	if(value.isValid())	setValue(IDS_STATUSTOOLTIP, value);

	value = extSettings.value(IDS_SENDKEYMOD);
	if(value.isValid())	setValue(IDS_SENDKEYMOD, value);

	setValue(IDS_VERSION, IDA_VERSION);
	sync();

	QFile::remove(StdLocation::tempConfigFile());

	return true;
}

void lmcSettings::setAutoStart(bool on) {
#ifdef Q_WS_WIN
	QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
		QSettings::NativeFormat);
	if(on)
		settings.setValue(IDA_TITLE, QDir::toNativeSeparators(QApplication::applicationFilePath()));
	else
		settings.remove(IDA_TITLE);
#endif

#ifdef Q_WS_MAC
    Q_UNUSED(on);
#endif

#ifdef Q_WS_X11
	//  get the path of .desktop file
	QString autoStartDir;
	char* buffer = getenv("XDG_CONFIG_HOME");
	if(buffer) {
		autoStartDir = QString(buffer);
		autoStartDir.append("/autostart");
	} else {
		buffer = getenv("HOME");
		autoStartDir = QString(buffer);
		autoStartDir.append("/.config/autostart");
	}
	QDir dir(autoStartDir);
	QString fileName = dir.absoluteFilePath("lmc.desktop");
	//	delete the file if autostart is set to false
	if(!on) {
		QFile::remove(fileName);
		return;
	}

	if(!dir.exists())
		dir.mkpath(dir.absolutePath());
	QFile file(fileName);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;
	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	stream.setGenerateByteOrderMark(false);
	stream << "[Desktop Entry]\n";
	stream << "Encoding=UTF-8\n";
	stream << "Type=Application\n";
	stream << "Name=" << IDA_TITLE << "\n";
	stream << "Comment=Send and receive instant messages\n";
	stream << "Icon=lmc\n";
	stream << "Exec=sh " << qApp->applicationDirPath() << "/lmc.sh\n";
	stream << "Terminal=false\n";
	file.close();
#endif
}

//	The function expects the config file to exist. Validation must be done
//	prior to calling the function.
bool lmcSettings::migrateSettings(const QString& configFile) {
	lmcSettingsBase settings(configFile, QSettings::IniFormat);

	QString version = settings.value(IDS_VERSION, IDS_VERSION_VAL).toString();

	//	Check if settings can be migrated, else reset settings and return false
	//	If the settings are from a later version, its deemed non migratable
	if(Helper::compareVersions(IDA_VERSION, version) < 0) {
		QFile::remove(configFile);
		return false;
	}

	//	Migrate settings from version 1.2.10 and older
	//	Any version before 1.2.10 will also return 1.2.10 as the version since its the
	//	default value
	if(Helper::compareVersions(version, "1.2.10") == 0) {
		//	The group settings were changed in v1.2.16
		//	Groups now have both an id and a name. All the existing groups in the
		//	settings are assigned a unique id. The general group is assigned a special
		//	unique id that is predefined. The user group mapping is done with user id
		//	and group id.
		QList<Group> groupList;
		QHash<QString, QString> groupIdHash;
		QMap<QString, QString> userGroupMap;

		int size = settings.beginReadArray(IDS_GROUPHDR);
		for(int index = 0; index < size; index++) {
			settings.setArrayIndex(index);
			QString groupName = settings.value(IDS_GROUP).toString();
			QString groupId = (groupName == GRP_DEFAULT) ? GRP_DEFAULT_ID : Helper::getUuid();
			groupList.append(Group(groupId, groupName));
			groupIdHash.insert(groupName, groupId);
		}
		settings.endArray();

		if(groupList.count() == 0)
			groupList.append(Group(GRP_DEFAULT_ID, GRP_DEFAULT));

		size = settings.beginReadArray(IDS_GROUPMAPHDR);
		for(int index = 0; index < size; index++)
		{
			settings.setArrayIndex(index);
			QString userId = settings.value(IDS_USER).toString();
			QString groupName = settings.value(IDS_GROUP).toString();
			QString groupId = groupIdHash.value(groupName);
			userGroupMap.insert(userId, groupId);
		}
		settings.endArray();

		// now save settings in the new format
		settings.beginWriteArray(IDS_GROUPHDR);
		for(int index = 0; index < groupList.count(); index++) {
			settings.setArrayIndex(index);
			settings.setValue(IDS_GROUP, groupList[index].id);
			settings.setValue(IDS_GROUPNAME, groupList[index].name);
		}
		settings.endArray();

		settings.beginWriteArray(IDS_GROUPMAPHDR);
		QMapIterator<QString, QString> i(userGroupMap);
		int count = 0;
		while(i.hasNext()) {
			settings.setArrayIndex(count);
			i.next();
			settings.setValue(IDS_USER, i.key());
			settings.setValue(IDS_GROUP, i.value());
			count++;
		}
		settings.endArray();
	}
	//	End of migration from 1.2.10

	//	Migrate settings if version less than 1.2.25
	if(Helper::compareVersions(version, "1.2.25") < 0) {
		//	Theme is now saved under the "Appearance" section
		//	Prior to 1.2.25, it was under "Themes" section which is no longer used
		QString theme = settings.value(IDS_THEME_OLD).toString();
		if(!theme.isEmpty()) settings.setValue(IDS_THEME, theme, IDS_THEME_VAL);

		//	Change the refresh interval to reduce network load
		int refreshTime = settings.value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt();
		if(refreshTime < IDS_REFRESHTIME_VAL) settings.setValue(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL);

		//	Group settings (except group tree item expanded info) are now saved in a
		//	separate group file. Copy group details from settings file to group file
		QHash<QString, QString> groupIdHash;
		QMap<QString, QString> userGroupMap;

		int size = settings.beginReadArray(IDS_GROUPHDR);
		for(int index = 0; index < size; index++) {
			settings.setArrayIndex(index);
			QString groupId = settings.value(IDS_GROUP).toString();
			QString groupName = settings.value(IDS_GROUPNAME).toString();
			groupIdHash.insert(groupId, groupName);
		}
		settings.endArray();

		size = settings.beginReadArray(IDS_GROUPMAPHDR);
		for(int index = 0; index < size; index++)
		{
			settings.setArrayIndex(index);
			QString user = settings.value(IDS_USER).toString();
			QString group = settings.value(IDS_GROUP).toString();
			userGroupMap.insert(user, group);
		}
		settings.endArray();

		QSettings groupSettings(StdLocation::groupFile(), QSettings::IniFormat);
		groupSettings.beginWriteArray(IDS_GROUPHDR);
		QHashIterator<QString, QString> it(groupIdHash);
		int count = 0;
		while(it.hasNext()) {
			groupSettings.setArrayIndex(count);
			it.next();
			groupSettings.setValue(IDS_GROUP, it.key());
			groupSettings.setValue(IDS_GROUPNAME, it.value());
			count++;
		}
		groupSettings.endArray();

		groupSettings.beginWriteArray(IDS_GROUPMAPHDR);
		QMapIterator<QString, QString> i(userGroupMap);
		count = 0;
		while(i.hasNext()) {
			groupSettings.setArrayIndex(count);
			i.next();
			groupSettings.setValue(IDS_USER, i.key());
			groupSettings.setValue(IDS_GROUP, i.value());
			count++;
		}
		groupSettings.endArray();
	}
	//	End of migration to 1.2.25

	//	Migrate settings if version less than 1.2.28
	if(Helper::compareVersions(version, "1.2.28") < 0) {
		//	Broadcast list used instead of single broadcast address
		//	Remove old entry from settings
		settings.remove(IDS_BROADCAST_OLD);
	}
	//	End of migration to 1.2.28

	// Migrate settings if version less than 1.2.30
	if(Helper::compareVersions(version, "1.2.30") < 0) {
		//	The old group settings were still present in the settings file though
		//	group settings are now saved to a different file.
		//	Remove group settings from application settings file
		settings.beginGroup(IDS_GROUPHDR);
		settings.remove("");
		settings.endGroup();

		settings.beginGroup(IDS_GROUPMAPHDR);
		settings.remove("");
		settings.endGroup();
	}

	settings.setValue(IDS_VERSION, IDA_VERSION);
	settings.sync();
	return true;
}
