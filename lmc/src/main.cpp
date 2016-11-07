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


#include "application.h"
#include "lmc.h"
#include <QResource>
#include <QMessageBox>

//	Define a unique application id. This is a combination of two GUIDs
const QString appId = "93fab548-2cf5-4a1e-8758-a416a5ec2120-6fc5009f-84e8-4489-a444-7f934bcf9166";

int showSwitches(void) {
	QString msg =	"Usage:	lmc [switches]\n" \
					"	All switches are optional.\n" \
					"\n" \
					"/loopback - Allows loopback communication with local machine.\n" \
					"/nohistory - Deletes existing history.\n" \
					"/nofilehistory - Deletes existing file transfer history.\n" \
					"/noconfig - Resets preferences to default values.\n" \
					"/sync - Synchronizes application settings and their external dependencies.\n" \
					"/unsync - Removes all external application dependencies.\n" \
					"/term - Closes an instance which is already running.\n" \
					"/inst - Returns a value indicating whether an instance is already running.\n" \
					"/quit - Closes once the command line parameters are processed.\n" \
					"/? - Display this help.\n" \
					"\n" \
					"Example:\n" \
					"Start in loopback mode with default settings: lmc /loopback /noconfig\n" \
					"\n" \
					"Some command line switches are mutually exclusive. If multiple switches\n" \
					"are specified, they will take precedence in the order given above.\n" \
					"Copyright (C) 2010-2011 Dilip Radhakrishnan.\n";
	QMessageBox::information(NULL, IDA_TITLE, msg, QMessageBox::Ok);
	return 0;
}

int main(int argc, char *argv[]) {
	Application application(appId, argc, argv);
	QDir::setCurrent(application.applicationDirPath());
	QStringList libPaths;
	libPaths.append(application.applicationDirPath());
	QApplication::setLibraryPaths(libPaths);
	QResource::registerResource("lmc.rcc");
	QApplication::setWindowIcon(QIcon(IDR_APPICON));

	QString messageList;
	QString message;

	for(int index = 1; index < argc; index++) {
		message = QString(argv[index]).toLower();
		if(message.compare("/?") == 0)
			return showSwitches();
		else if(message.compare("/inst") == 0)
			return application.isRunning() ? 1 : 0;
		else
			messageList += message + " ";
	}

	if(application.sendMessage(messageList))
		return 0;
	
	application.loadTranslations(application.applicationDirPath() + "/lang");

	lmcCore core;
	core.init();
	messageList += "/new";	//	indicates this is a new instance
	if(!core.receiveAppMessage(messageList))	//	handle command line args if this is first instance
		return 0;
	if(!core.start())
		return 1;

	QObject::connect(&application, SIGNAL(messageReceived(const QString&)),
		&core, SLOT(receiveAppMessage(const QString&)));
	QObject::connect(&application, SIGNAL(aboutToQuit()), &core, SLOT(aboutToExit()));

	return application.exec();
}
