// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "gui/chat/chattextguard.h"

#include <QtMath>

namespace ChatTextGuard {

namespace {

double linearChannel(double c8) {
  double cs = c8 / 255.0;
  if(cs <= 0.03928)
    return cs / 12.92;
  return qPow((cs + 0.055) / 1.055, 2.4);
}

double luminance(const QColor& c) {
  return 0.2126 * linearChannel(c.red()) + 0.7152 * linearChannel(c.green()) +
         0.0722 * linearChannel(c.blue());
}

} // namespace

double contrastRatio(const QColor& a, const QColor& b) {
  double la = luminance(a);
  double lb = luminance(b);
  double lighter = qMax(la, lb);
  double darker = qMin(la, lb);
  return (lighter + 0.05) / (darker + 0.05);
}

bool textReadableOn(const QColor& fg, const QColor& bg) {
  if(!fg.isValid() || !bg.isValid())
    return false;
  if(fg.alpha() == 0 || bg.alpha() == 0)
    return false;
  return contrastRatio(fg, bg) >= kMinContrast;
}

QFont sanitizeIncomingFont(const QFont& font) {
  QFont f = font;
  double pt = f.pointSizeF();
  if(pt > 0.0) {
    f.setPointSizeF(qBound(kMinFontPt, pt, kMaxFontPt));
    return f;
  }
  //  fromString may carry pixelSize instead of pointSize (pointSizeF() == -1):
  //  normalize to pt at 96 dpi, then clamp. Fully broken sizes stay for
  //  rowFont to map to the theme size.
  if(f.pixelSize() > 0) {
    double asPt = f.pixelSize() * 72.0 / 96.0;
    f.setPointSizeF(qBound(kMinFontPt, asPt, kMaxFontPt));
  }
  return f;
}

QColor sanitizeIncomingColor(const QColor& color) {
  if(!color.isValid())
    return color;
  if(color.alpha() == 255)
    return color;
  QColor opaque = color;
  opaque.setAlpha(255);
  return opaque;
}

} // namespace ChatTextGuard
