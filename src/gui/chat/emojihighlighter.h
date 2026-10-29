// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QSyntaxHighlighter>

class QTextDocument;

//  Composer highlighter: emoji runs get the color-emoji families,
//  the rest keeps the widget font.
class EmojiHighlighter : public QSyntaxHighlighter {
  Q_OBJECT

public:
  explicit EmojiHighlighter(QTextDocument* document);
  //  Base family for the monochrome fallback; widget font, not defaultFont().
  void setBaseFamily(const QString& family);

protected:
  void highlightBlock(const QString& text) override;

private:
  QString baseFamily;
};
