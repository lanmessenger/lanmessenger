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


#include "settings.h"

//	migrate settings from older versions to new format
bool lmcSettings::migrateSettings(void) {
	QString version = value(IDS_VERSION, IDS_VERSION_VAL).toString();

	//	Check if settings can be migrated, else reset settings and return false
	//	If the settings are from a later version, its deemed non migratable
	if(Helper::compareVersions(IDA_VERSION, version) < 0) {
		QFile::remove(fileName());
		sync();
		return false;
	}

	//	Migrate settings from version 1.2.10
	if(Helper::compareVersions(version, "1.2.10") == 0) {
		QList<Group> groupList;
		QHash<QString, QString> groupIdHash;
		QMap<QString, QString> userGroupMap;

		int size = beginReadArray(IDS_GROUPHDR);
		for(int index = 0; index < size; index++) {
			setArrayIndex(index);
			QString groupName = value(IDS_GROUP).toString();
			QString groupId = (groupName == GRP_DEFAULT) ? GRP_DEFAULT_ID : Helper::getUuid();
			groupList.append(Group(groupId, groupName));
			groupIdHash.insert(groupName, groupId);
		}
		endArray();

		if(groupList.count() == 0)
			groupList.append(Group(GRP_DEFAULT_ID, GRP_DEFAULT));

		size = beginReadArray(IDS_GROUPMAPHDR);
		for(int index = 0; index < size; index++)
		{
			setArrayIndex(index);
			QString userId = value(IDS_USER).toString();
			QString groupName = value(IDS_GROUP).toString();
			QString groupId = groupIdHash.value(groupName);
			userGroupMap.insert(userId, groupId);
		}
		endArray();

		// now save settings in the new format
		beginWriteArray(IDS_GROUPHDR);
		for(int index = 0; index < groupList.count(); index++) {
			setArrayIndex(index);
			setValue(IDS_GROUP, groupList[index].id);
			setValue(IDS_GROUPNAME, groupList[index].name);
		}
		endArray();

		beginWriteArray(IDS_GROUPMAPHDR);
		QMapIterator<QString, QString> i(userGroupMap);
		int count = 0;
		while(i.hasNext()) {
			setArrayIndex(count);
			i.next();
			setValue(IDS_USER, i.key());
			setValue(IDS_GROUP, i.value());
			count++;
		}
		endArray();
	}
	// End of migration from 1.2.10

	setValue(IDS_VERSION, IDA_VERSION);
	sync();
	return true;
}

void lmcSettings::setAutoStart(bool on) {
#if defined Q_WS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        QSettings::NativeFormat);
    if(on)
        settings.setValue(IDA_TITLE, QDir::toNativeSeparators(QApplication::applicationFilePath()));
    else
        settings.remove(IDA_TITLE);
#elif defined Q_WS_MAC
	Q_UNUSED(on);
#elif defined Q_WS_X11
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
