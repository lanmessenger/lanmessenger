// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QColor>
#include <QFont>
#include <QString>
#include <QVariant>

#include "core/theme.h"

//  Pure HTML helpers for the chat row body.
class RowHtmlBuilder {
public:
  static QString fontStyleFor(const QFont& font, const QColor& color, int fontSizeVal, bool own,
                              bool fontExplicit);
  static QFont messageFont(const QVariant& fontVar, double themeSize);
  static QString decodeBody(QString text, bool trim, bool allowLinks, bool pathToLink,
                            const QString& baseFamily = QString());
  //  Explicit color wins unless it fails contrast; then the row color.
  static QColor guardedColor(const QColor& explicitColor, const Theme::Bg& bg,
                             const Theme::DualColor& fallbackText, const Theme::DualColor& page,
                             bool darkMode);
};
