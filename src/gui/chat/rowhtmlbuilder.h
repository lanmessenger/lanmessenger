// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QColor>
#include <QFont>
#include <QString>
#include <QVariant>

#include "core/theme.h"

//  Pure HTML helpers for the chat row body. No model, no tr() here:
//  fileCard captions stay in BubbleDelegate to keep the translation
//  context ("BubbleDelegate") and .ts files untouched.
class RowHtmlBuilder {
public:
  static QString fontStyleFor(const QFont& font, const QColor& color, int fontSizeVal, bool own,
                              bool fontExplicit);
  static QFont messageFont(const QVariant& fontVar, double themeSize);
  static QString decodeBody(QString text, bool trim, bool allowLinks, bool pathToLink,
                            bool showSmiley);
  //  Explicit sender color wins unless it fails the contrast floor
  //  against the bubble fill; then fall back to the row text color.
  static QColor guardedColor(const QColor& explicitColor, const Theme::Bg& bg,
                             const Theme::DualColor& fallbackText, const Theme::DualColor& page,
                             bool darkMode);
};
