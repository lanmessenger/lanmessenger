// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef THEME_H
#define THEME_H

#include "stdlocation.h"

struct Theme {
  QString name;
  QString path;

  Theme() {}
  Theme(QString szName, QString szPath) {
    name = szName;
    path = szPath;
  }
};

typedef QList<Theme> Themes;

struct ThemeData {
  QString themePath;
  QString styleSheet;
  QString inMsg;
  QString inNextMsg;
  QString outMsg;
  QString outNextMsg;
  QString pubMsg;
  QString sysMsg;
  QString sysNextMsg;
  QString reqMsg;
  QString stateMsg;
};

class lmcTheme {
public:
  static const Themes availableThemes(void);
  static const ThemeData loadTheme(const QString& path);
};

#endif // THEME_H
