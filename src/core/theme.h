// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QColor>
#include <QJsonArray>
#include <QMap>
#include <QString>

//  Native chat render (Bubble): tokens + light/dark + layout_*.
//  Stage 1 parses and validates layout_* but ignores them (delegate tree is
//  hardcoded 1:1 with them). Stage 2 reads the same file.
class Theme {
public:
  enum class Mode {
    System,
    Light,
    Dark
  };

  struct Bubble {
    int radius[4] = {12, 12, 12, 4};
    int tail = 8;
    double shadowDx = 0.0;
    double shadowDy = 2.0;
    double shadowBlur = 8.0;
    double shadowOpacity = 0.18;
    int padding[4] = {8, 10, 8, 10};
    int spacing = 6;
    int groupedSpacing = 2;
    int maxWidthPct = 75;
  };

  struct Avatar {
    int size = 40;
    QString shape = QStringLiteral("circle");
    bool showOnGrouped = false;
  };

  struct Header {
    bool senderBold = true;
    QString timeFormat = QStringLiteral("HH:mm");
  };

  struct Card {
    QString bg;
    QString bg2;
    QString border;
    QString text;
    QString tail = QStringLiteral("none");
    QString align;
    bool avatar = false;
    bool hasAvatar = false;
  };

  bool isValid = false;
  QString errorString;
  Bubble bubble;
  Avatar avatar;
  Header header;
  QMap<QString, QColor> light;
  QMap<QString, QColor> dark;
  QMap<QString, Card> cards;
  //  Raw layout trees for stage 2 (stage 1 only validates them).
  QMap<QString, QJsonArray> layouts;

  static Theme loadFromJson(const QByteArray& data);
  static Theme loadFromFile(const QString& filePath);
  static Theme builtInDefault();
  static QString defaultJsonText();
  //  Theme discovery: validated names, user dir overrides built-in qrc.
  static QStringList availableThemes();
  //  Resolve a theme name to its theme.json (empty when unknown).
  static QString themeFileFor(const QString& name);

  bool hasToken(const QString& token) const;
  //  Mode::System resolves via systemDark; Light/Dark ignore it.
  QColor resolve(const QString& ref, Mode mode, bool systemDark) const;
  QColor resolveToken(const QString& token, bool darkMode) const;
  bool isDark(Mode mode, bool systemDark) const;
  QJsonArray layoutFor(const QString& name) const;
};
