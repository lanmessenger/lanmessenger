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
};

#endif // STDLOCATION_H