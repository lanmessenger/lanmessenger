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

#define IDS_LANGUAGE_VAL "en_US"

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
	// Locales found in this directory. System translations below are loaded
	// per directory (not from the global hash), so user-supplied
	// <userLangDir>/system/*.qm keep working.
	QStringList locales;
	for (QFileInfo file : entries) {
		// pick country and language out of the file name
		QStringList parts = file.baseName().split("_");
		if (parts.count() < 2)
			continue;
		QString language = parts.at(parts.count() - 2).toLower();
		QString country  = parts.at(parts.count() - 1).toUpper();

		// construct and load translator
		QTranslator* translator = new QTranslator(instance());
		if (translator->load(file.absoluteFilePath())) {
			QString locale = language + "_" + country;
			delete translators.take(locale);
			translators.insert(locale, translator);
			if (!locales.contains(locale))
				locales.append(locale);
		}
		else
			delete translator;
	}
	// en_US is the source language: keep the NULL marker (no app translator),
	// dropping the just-loaded identity translator if any.
	delete translators.take(IDS_LANGUAGE_VAL);
	translators.insert(IDS_LANGUAGE_VAL, NULL);

	// Qt's own translations for the standard dialogs (qtbase_*.qm, bundled at
	// build time): country specific file first (qtbase_pt_BR.qm), language
	// only as fallback (qtbase_pt.qm).
	QDir sysDir(dir.absolutePath() + "/system");
	if(!sysDir.exists())
		return;

	for (const QString& locale : locales) {
		// en_US is the source language (NULL translator); there is no
		// qtbase_en*.qm for it, mirroring the CMake skip.
		if (locale == IDS_LANGUAGE_VAL)
			continue;
		QStringList parts = locale.split("_");
		QString language = QString("qtbase_%1").arg(parts.value(0).toLower());
		QString country = QString("qtbase_%1_%2")
			.arg(parts.value(0).toLower(), parts.value(1).toUpper());

		QTranslator* translator = new QTranslator(instance());
		if (translator->load(country, sysDir.absolutePath()) ||
			translator->load(language, sysDir.absolutePath())) {
			delete sysTranslators.take(locale);
			sysTranslators.insert(locale, translator);
		}
		else
			delete translator;
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
