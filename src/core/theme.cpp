// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "theme.h"

const QString defTheme = StdLocation::resThemeDir() + "/Classic";

const QString docTemplate(
  "<html>"
  "<head>"
  "<style type='text/css'>"
  "%1"
  "</style>"
  "</head>"
  "<body style='-webkit-nbsp-mode: space; word-wrap:break-word;'>"
  "</body>"
  "</html>");


const Themes lmcTheme::availableThemes(void) {
  QDir::Filters filters = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable;
  QDir::SortFlags sort = QDir::Name;

  Themes themes;

  QDir dir(StdLocation::resThemeDir());
  QStringList entries = dir.entryList(QStringList(), filters, sort);
  for(QString dirName : entries) {
    themes.append(Theme(dirName, dir.absoluteFilePath(dirName)));
  }

  dir.setPath(StdLocation::sysThemeDir());
  entries = dir.entryList(QStringList(), filters, sort);
  for(QString dirName : entries) {
    themes.append(Theme(dirName, dir.absoluteFilePath(dirName)));
  }

  dir.setPath(StdLocation::userThemeDir());
  entries = dir.entryList(QStringList(), filters, sort);
  for(QString dirName : entries) {
    themes.append(Theme(dirName, dir.absoluteFilePath(dirName)));
  }

  return themes;
}

const ThemeData lmcTheme::loadTheme(const QString& path) {
  QFile file;
  ThemeData themeData;

  themeData.themePath = path;

  //	Style sheet is optional: themes without main.css (e.g. Classic which
  //	uses inline styles) simply get an empty style sheet. No fallback to
  //	the default theme here, a missing file is not an error.
  file.setFileName(path + "/main.css");
  if(file.open(QIODevice::ReadOnly)) {
    themeData.styleSheet = QString(file.readAll().constData());
    file.close();
  } else {
    themeData.styleSheet.clear();
  }

  file.setFileName(path + "/Incoming/Content.html");
  if(!file.open(QIODevice::ReadOnly))
    return loadTheme(defTheme);
  themeData.inMsg = QString(file.readAll().constData());
  file.close();

  file.setFileName(path + "/Incoming/NextContent.html");
  if(!file.open(QIODevice::ReadOnly))
    return loadTheme(defTheme);
  themeData.inNextMsg = QString(file.readAll().constData());
  file.close();

  file.setFileName(path + "/Outgoing/Content.html");
  if(!file.open(QIODevice::ReadOnly))
    return loadTheme(defTheme);
  themeData.outMsg = QString(file.readAll().constData());
  file.close();

  file.setFileName(path + "/Outgoing/NextContent.html");
  if(!file.open(QIODevice::ReadOnly))
    return loadTheme(defTheme);
  themeData.outNextMsg = QString(file.readAll().constData());
  file.close();

  file.setFileName(path + "/Broadcast.html");
  if(!file.open(QIODevice::ReadOnly))
    return loadTheme(defTheme);
  themeData.pubMsg = QString(file.readAll().constData());
  file.close();

  file.setFileName(path + "/Status.html");
  if(!file.open(QIODevice::ReadOnly))
    return loadTheme(defTheme);
  QString sys = QString(file.readAll().constData());
  themeData.sysMsg = sys;
  themeData.stateMsg = sys;
  file.close();

  file.setFileName(path + "/NextStatus.html");
  if(!file.open(QIODevice::ReadOnly))
    return loadTheme(defTheme);
  themeData.sysNextMsg = QString(file.readAll().constData());
  file.close();

  file.setFileName(path + "/Request.html");
  if(!file.open(QIODevice::ReadOnly))
    return loadTheme(defTheme);
  themeData.reqMsg = QString(file.readAll().constData());
  file.close();

  return themeData;
}
