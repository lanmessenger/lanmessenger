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


#include "theme.h"

const QString defTheme = StdLocation::resThemeDir() + "/Classic";

const QString docTemplate(
	"<html>"\
	"<head>"\
	"<style type='text/css'>"\
		"%1"\
	"</style>"\
	"</head>"\
	"<body style='-webkit-nbsp-mode: space; word-wrap:break-word;'>"\
	"</body>"\
	"</html>");


const Themes lmcTheme::availableThemes(void) {
	QDir::Filters filters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable;
	QDir::SortFlags sort = QDir::Name;

	Themes themes;

	QDir dir(StdLocation::resThemeDir());
	QStringList entries = dir.entryList(QStringList(), filters, sort);
	foreach(QString dirName, entries) {
		themes.append(Theme(dirName, dir.absoluteFilePath(dirName)));
	}

	dir.setPath(StdLocation::sysThemeDir());
	entries = dir.entryList(QStringList(), filters, sort);
	foreach(QString dirName, entries) {
		themes.append(Theme(dirName, dir.absoluteFilePath(dirName)));
	}

	dir.setPath(StdLocation::userThemeDir());
	entries = dir.entryList(QStringList(), filters, sort);
	foreach(QString dirName, entries) {
		themes.append(Theme(dirName, dir.absoluteFilePath(dirName)));
	}

	return themes;
}

const ThemeData lmcTheme::loadTheme(const QString &path) {
	QFile file;
	ThemeData themeData;

	themeData.themePath = path;

	file.setFileName(path + "/main.css");
	if(!file.open(QIODevice::ReadOnly))
		return loadTheme(defTheme);
	QString style = QString(file.readAll().constData());
	themeData.document = docTemplate.arg(style);
	file.close();

	file.setFileName(path + "/Incoming/Content.html");
	if(!file.open(QIODevice::ReadOnly))
		return loadTheme(defTheme);
	themeData.inMsg = "<div class='_lmc_chatdiv'>" + QString(file.readAll().constData()) + "</div>";
	file.close();

	file.setFileName(path + "/Incoming/NextContent.html");
	if(!file.open(QIODevice::ReadOnly))
		return loadTheme(defTheme);
	themeData.inNextMsg = QString(file.readAll().constData());
	file.close();

	file.setFileName(path + "/Outgoing/Content.html");
	if(!file.open(QIODevice::ReadOnly))
		return loadTheme(defTheme);
	themeData.outMsg = "<div class='_lmc_chatdiv'>" + QString(file.readAll().constData()) + "</div>";
	file.close();

	file.setFileName(path + "/Outgoing/NextContent.html");
	if(!file.open(QIODevice::ReadOnly))
		return loadTheme(defTheme);
	themeData.outNextMsg = QString(file.readAll().constData());
	file.close();

	file.setFileName(path + "/Broadcast.html");
	if(!file.open(QIODevice::ReadOnly))
		return loadTheme(defTheme);
	themeData.pubMsg = "<div class='_lmc_publicdiv'>" + QString(file.readAll().constData()) + "</div>";
	file.close();

	file.setFileName(path + "/Status.html");
	if(!file.open(QIODevice::ReadOnly))
		return loadTheme(defTheme);
	QString sys = QString(file.readAll().constData());
	themeData.sysMsg = "<div class='_lmc_sysdiv'>" + sys + "</div>";
	themeData.stateMsg = "<div class='_lmc_statediv'>" + sys + "</div>";
	file.close();

	file.setFileName(path + "/NextStatus.html");
	if(!file.open(QIODevice::ReadOnly))
		return loadTheme(defTheme);
	themeData.sysNextMsg = QString(file.readAll().constData());
	file.close();

	file.setFileName(path + "/Request.html");
	if(!file.open(QIODevice::ReadOnly))
		return loadTheme(defTheme);
	themeData.reqMsg = "<div class='_lmc_reqdiv'>" + QString(file.readAll().constData()) + "</div>";
	file.close();

	return themeData;
}
