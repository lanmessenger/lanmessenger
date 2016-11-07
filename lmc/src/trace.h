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


#ifndef TRACE_H
#define TRACE_H

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include "stdlocation.h"
#include "xmlmessage.h"

class lmcTrace {
public:
	lmcTrace(void);
	~lmcTrace(void);

	static void init(XmlMessage* pInitParams);
    static void write(const QString& string, bool verbose = true);

private:
	static bool traceMode;
	static QString fileName;
};

#endif // TRACE_H
