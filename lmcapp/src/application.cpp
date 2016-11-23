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

#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include "application.h"

QTranslator* Application::current = 0;
QTranslator* Application::sysCurrent = 0;
Translators Application::translators;
Translators Application::sysTranslators;

Application::Application(const QString& id, int& argc, char** argv) 
	: QtSingleApplication(id, argc, argv) {
}

Application::~Application(void) {
}

void Application::loadTranslations(const QString& dir) {
	loadTranslations(QDir(dir));
}

void Application::loadTranslations(const QDir& dir) {
	// <language>_<country>
	QString filter = "*_*.qm";
	QDir::Filters filters = QDir::Files | QDir::Readable;
	QDir::SortFlags sort = QDir::Name;
	QFileInfoList entries = dir.entryInfoList(QStringList() << filter, filters, sort);
	foreach (QFileInfo file, entries) {
		// pick country and language out of the file name
		QStringList parts = file.baseName().split("_");
		QString language = parts.at(parts.count() - 2).toLower();
		QString country  = parts.at(parts.count() - 1).toUpper();

		// construct and load translator
		QTranslator* translator = new QTranslator(instance());
		if (translator->load(file.absoluteFilePath())) {
			QString locale = language + "_" + country;
			translators.insert(locale, translator);
		}
	}
        translators.insert(IDS_LANGUAGE_VAL, NULL);

	// load system translations
	QDir sysDir(dir.absolutePath() + "/system");
	if(!sysDir.exists())
		return;

	entries = sysDir.entryInfoList(QStringList() << filter, filters, sort);
	foreach (QFileInfo file, entries) {
		// pick country and language out of the file name
		QStringList parts = file.baseName().split("_");
		QString language = parts.at(parts.count() - 2).toLower();
		QString country  = parts.at(parts.count() - 1).toUpper();

		// construct and load translator
		QTranslator* translator = new QTranslator(instance());
		if (translator->load(file.absoluteFilePath())) {
			QString locale = language + "_" + country;
			sysTranslators.insert(locale, translator);
		}
	}
}

const QStringList Application::availableLanguages() {
	// the content won't get copied thanks to implicit sharing and constness
	return QStringList(translators.keys());
}

void Application::setLanguage(const QString& locale) {
	// remove previous translator
	if(current)
		removeTranslator(current);
	if(sysCurrent)
		removeTranslator(sysCurrent);

	// install new translator for the selected locale
	// when multiple translators are included, translations are fetched
	// in the reverse order. ie, last translator is searched first.
	// install system translations first
	sysCurrent = sysTranslators.value(locale, 0);
	if(sysCurrent)
		installTranslator(sysCurrent);
	// now install application translations
	current = translators.value(locale, 0);
	if(current)
		installTranslator(current);
}
