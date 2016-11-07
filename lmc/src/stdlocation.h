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


#ifndef STDLOCATION_H
#define STDLOCATION_H

#include <QDir>
#include <QDesktopServices>
#include "settings.h"

#define SL_TRANSFERHISTORY		"transfers.lst"
#define SL_FILESTORAGEDIR		"Received Files"
#define SL_CACHEDIR				"cache"
#define SL_RESOURCE				"lmc.rcc"
#define SL_LANGDIR				"lang"
#define SL_THEMEDIR				"themes"

class StdLocation {
public:
	static QString transferHistory(void) {
		return QDir::toNativeSeparators(QDesktopServices::storageLocation(
			QDesktopServices::DataLocation) + "/"SL_TRANSFERHISTORY);
	}

	static QString fileStorageDir(void) {
		lmcSettings settings;

		QString path = QDir::toNativeSeparators(QDesktopServices::storageLocation(
			QDesktopServices::DocumentsLocation) + "/"SL_FILESTORAGEDIR);
		path = settings.value(IDS_FILESTORAGEPATH, path).toString();
		return path;
	}

	static QString cacheDir(void) {
		return QDir::toNativeSeparators(QDesktopServices::storageLocation(
			QDesktopServices::DataLocation) + "/"SL_CACHEDIR);
	}

	static QString libDir(void) {
		return QDir::toNativeSeparators(QDir::currentPath());
	}

	static QString resourceFile(void) {
		return QDir::toNativeSeparators(QDir::current().absoluteFilePath(SL_RESOURCE));
	}

	static QString resLangDir(void) {
		return ":/"SL_LANGDIR;
	}

	static QString sysLangDir(void) {
		return QDir::toNativeSeparators(QDir::currentPath() + "/"SL_LANGDIR);
	}

	static QString userLangDir(void) {
		return QDir::toNativeSeparators(QDesktopServices::storageLocation(
			QDesktopServices::DataLocation) + "/"SL_LANGDIR);
	}

	static QString resThemeDir(void) {
		return ":/"SL_THEMEDIR;
	}

	static QString sysThemeDir(void) {
		return QDir::toNativeSeparators(QDir::currentPath() + "/"SL_THEMEDIR);
	}

	static QString userThemeDir(void) {
		return QDir::toNativeSeparators(QDesktopServices::storageLocation(
			QDesktopServices::DataLocation) + "/"SL_THEMEDIR);
	}
};

#endif // STDLOCATION_H
