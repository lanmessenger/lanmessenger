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


#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QDir>
#include <QDesktopServices>
#include "settings.h"

#define FC_FILENAME		"transfers.lst"
#define FC_STORAGE		"Received Files"

class FileTransfer {
public:
	static QString historyFile(void) {
		return QDir::toNativeSeparators(QDesktopServices::storageLocation(
			QDesktopServices::DataLocation) + "/"FC_FILENAME);
	}

	static QString storageFolder(void) {
		lmcSettings settings;

		QString path = QDir::toNativeSeparators(QDesktopServices::storageLocation(
			QDesktopServices::DocumentsLocation) + "/"FC_STORAGE);
		path = settings.value(IDS_FILESTORAGEPATH, path).toString();
		return path;
	}
};

#endif // FILETRANSFER_H