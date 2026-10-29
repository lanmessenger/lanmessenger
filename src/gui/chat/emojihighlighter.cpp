// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "gui/chat/emojihighlighter.h"

#include <QTextCharFormat>
#include <QTextDocument>

#include "gui/chat/chathelper.h"

EmojiHighlighter::EmojiHighlighter(QTextDocument* document)
    : QSyntaxHighlighter(document) {
}

void EmojiHighlighter::setBaseFamily(const QString& family) {
  if(baseFamily == family)
    return;
  baseFamily = family;
  rehighlight();
}

void EmojiHighlighter::highlightBlock(const QString& text) {
  if(text.isEmpty())
    return;
  QTextCharFormat emojiFormat;
  //  Emoji-first, base last: color when the color font is installed,
  //  monochrome instead of tofu when it is not. Empty base keeps
  //  emoji-only spans, same as RowHtmlBuilder/wrapEmojiRuns.
  emojiFormat.setFontFamilies(ChatHelper::emojiFamiliesWithBase(baseFamily));
  int pos = 0;
  while(pos < text.length()) {
    int run = ChatHelper::emojiRunLength(text, pos);
    if(run > 0) {
      setFormat(pos, run, emojiFormat);
      pos += run;
    } else {
      //  Don't split surrogate pairs.
      if(text.at(pos).isHighSurrogate() && pos + 1 < text.length() &&
         text.at(pos + 1).isLowSurrogate())
        pos += 2;
      else
        pos += 1;
    }
  }
}
