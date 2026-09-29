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


#include "trace.h"

bool lmcTrace::traceMode;
QString lmcTrace::fileName;

lmcTrace::lmcTrace(void) {
}

lmcTrace::~lmcTrace(void) {
}

void lmcTrace::init(XmlMessage* pInitParams) {
	traceMode = Helper::stringToBool(pInitParams->data(XN_TRACEMODE));
	fileName = pInitParams->data(XN_LOGFILE);

	write("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n"\
          "         " IDA_TITLE " " IDA_VERSION " application log\n"\
		  "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
}

void lmcTrace::write(const QString& string, bool verbose) {
    if(!traceMode || !verbose)
		return;

	QDir logDir(StdLocation::logDir());
	if(!logDir.exists())
		logDir.mkdir(logDir.absolutePath());
	QFile file(fileName);
	if(!file.open(QIODevice::Text | QIODevice::Append))
		return;

	QTextStream stream(&file);

	QString timeStamp = "[" + QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss") + "] ";
	QStringList stringList = string.split("\n", Qt::SkipEmptyParts);
	for(int index = 0; index < stringList.count(); index++)
		stream << timeStamp << stringList[index] << "\n";

	file.close();
}
