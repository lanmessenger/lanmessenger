// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "gui/chat/rowhtmlbuilder.h"

#include <QRegularExpression>
#include <QVariant>

#include "core/uidefinitions.h"
#include "gui/chat/chathelper.h"
#include "gui/chat/chattextguard.h"

QString RowHtmlBuilder::fontStyleFor(const QFont& font, const QColor& color, int fontSizeVal,
                                     bool own, bool fontExplicit) {
  //  Base family only; emoji runs are covered by wrapEmojiRuns spans.
  QString style = QStringLiteral("font-family:\"%1\"; ").arg(font.family());
  if(font.italic())
    style += QStringLiteral("font-style:italic; ");
  if(font.bold())
    style += QStringLiteral("font-weight:bold; ");
  if(own || fontExplicit) {
    style += QStringLiteral("font-size:%1pt; ").arg(font.pointSizeF());
    if(color.isValid())
      style += QStringLiteral("color:%1; ").arg(color.name());
  } else {
    style += fontStyle[qBound(0, fontSizeVal, FS_COUNT - 1)] + QStringLiteral(" ");
    if(color.isValid())
      style += QStringLiteral("color:%1; ").arg(color.name());
  }
  if(font.strikeOut())
    style += QStringLiteral("text-decoration:line-through; ");
  if(font.underline())
    style += QStringLiteral("text-decoration:underline; ");
  return style;
}

QFont RowHtmlBuilder::messageFont(const QVariant& fontVar, double themeSize) {
  if(fontVar.isValid()) {
    QFont f = fontVar.value<QFont>();
    if(f.pointSizeF() <= 0)
      f.setPointSizeF(themeSize);
    else
      f.setPointSizeF(
        qBound(ChatTextGuard::kMinFontPt, f.pointSizeF(), ChatTextGuard::kMaxFontPt));
    return f;
  }
  QFont f;
  f.setPointSizeF(themeSize);
  return f;
}

QString RowHtmlBuilder::decodeBody(QString text, bool trim, bool allowLinks, bool pathToLink,
                                   const QString& baseFamily) {
  if(trim)
    text = text.trimmed();
  if(allowLinks) {
    text.replace(QRegularExpression(
                   QStringLiteral("((?:(?:https?|ftp|file)://|www\\.|ftp\\.)"
                                  "[-A-Z0-9+&@#/%=~_|$?!:,.]*[A-Z0-9+&@#/%=~_|$])"),
                   QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("<a data-isLink='true' href='\\1'>\\1</a>"));
    text.replace(QStringLiteral("<a data-isLink='true' href='www"),
                 QStringLiteral("<a data-isLink='true' href='http://www"));
    if(pathToLink)
      text.replace(QRegularExpression(QStringLiteral("((\\\\\\\\[\\w-]+\\\\[^\\\\/:*?<>|"
                                                     "]+)((?:\\\\[^\\\\/:*?<>|"
                                                     "]+)*\\\\?)$)")),
                   QStringLiteral("<a data-isLink='true' href='file:\\1'>\\1</a>"));
  }
  QString out;
  int pos = 0;
  while(pos < text.length()) {
    int start = text.indexOf(QStringLiteral("<a data-isLink='true'"), pos);
    if(start != -1) {
      QString segment = text.mid(pos, start - pos);
      ChatHelper::makeHtmlSafe(&segment);
      out += ChatHelper::wrapEmojiRuns(segment, baseFamily);
      int end = text.indexOf(QStringLiteral("</a>"), start) + 4;
      out += text.mid(start, end - start);
      pos = end;
    } else {
      QString segment = text.mid(pos);
      ChatHelper::makeHtmlSafe(&segment);
      out += ChatHelper::wrapEmojiRuns(segment, baseFamily);
      break;
    }
  }
  out.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
  return out;
}

QColor RowHtmlBuilder::guardedColor(const QColor& explicitColor, const Theme::Bg& bg,
                                    const Theme::DualColor& fallbackText,
                                    const Theme::DualColor& page, bool darkMode) {
  if(!explicitColor.isValid())
    return Theme::pick(fallbackText, darkMode);
  QColor bg1, bg2;
  Theme::pickBg(bg, darkMode, bg1, bg2);
  double worst = -1.0;
  if(bg1.alpha() != 0)
    worst = ChatTextGuard::contrastRatio(explicitColor, bg1);
  if(bg2.alpha() != 0) {
    double r = ChatTextGuard::contrastRatio(explicitColor, bg2);
    worst = (worst < 0.0) ? r : qMin(worst, r);
  }
  if(worst < 0.0) {
    QColor pageColor = Theme::pick(page, darkMode);
    if(pageColor.alpha() != 0)
      worst = ChatTextGuard::contrastRatio(explicitColor, pageColor);
  }
  if(worst >= 0.0 && worst < ChatTextGuard::kMinContrast)
    return Theme::pick(fallbackText, darkMode);
  return explicitColor;
}
