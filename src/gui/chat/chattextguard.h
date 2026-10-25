// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QColor>
#include <QFont>

//  Receive-side guard for peer-chosen XN_FONT/XN_COLOR (poisoning + contrast).
//  Pure QtGui helpers, no widgets. Wire is untouched. Applies to the local
//  echo too (same model data path).
namespace ChatTextGuard {

//  Absolute point-size clamp for incoming fonts. Out of range clamps to the
//  edge (not theme fallback); broken sizes (<= 0, pixelSize path handled
//  inside) are left for BubbleDelegate::rowFont to map to the theme size.
constexpr double kMinFontPt = 8.0;
constexpr double kMaxFontPt = 24.0;
//  WCAG contrast floor: an anti-invisible fuse, not AA 4.5.
constexpr double kMinContrast = 3.0;

//  WCAG 2.x contrast ratio of two opaque colors in [1.0, 21.0].
double contrastRatio(const QColor& a, const QColor& b);
bool textReadableOn(const QColor& fg, const QColor& bg);

//  Clamp a parsed incoming font to [kMinFontPt, kMaxFontPt]. Sizes <= 0 are
//  returned untouched (rowFont maps them to the local theme size); a point
//  size derived from pixelSize is normalized to pt first. Family and style
//  flags (bold/italic/underline/strikeout) pass through.
QFont sanitizeIncomingFont(const QFont& font);
//  Force opaque: transparent colors (e.g. "#00FFFFFF") would render invisible.
//  Invalid stays invalid (caller falls back to the theme color).
QColor sanitizeIncomingColor(const QColor& color);

} // namespace ChatTextGuard
