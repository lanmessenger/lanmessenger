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


#ifndef APPLICATION_H
#define APPLICATION_H

#include <qtsingleapplication.h>
#include <QHash>
#include <QStringList>
class QDir;
class QTranslator;

typedef QHash<QString, QTranslator*> Translators;

class QT_QTSINGLEAPPLICATION_EXPORT Application : public QtSingleApplication {
	Q_OBJECT

public:
	explicit Application(const QString& id, int& argc, char** argv);
	~Application(void);

	static void loadTranslations(const QString& dir);
	static void loadTranslations(const QDir& dir);
	static const QStringList availableLanguages();

public slots:
	static void setLanguage(const QString& locale);

private:
	static QTranslator* current;
	static QTranslator* sysCurrent;
	static Translators translators;
	static Translators sysTranslators;
};

#endif // APPLICATION_H
