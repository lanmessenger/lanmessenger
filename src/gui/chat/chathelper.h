// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QFont>
#include <QStringList>
#include <QTextBlockUserData>
#include "core/definitions.h"
#include "chatdefinitions.h"
#include "messaging/xmlmessage.h"

class QTextCharFormat;
class QTextEdit;

class QTextBlockData : public QTextBlockUserData {
public:
  QTextBlockData(QString id);
  virtual ~QTextBlockData();

  QString id;
};

struct SingleMessage {
  MessageType type;
  QString userId;
  QString userName;
  XmlMessage message;
  QString id;
  //  Lazy text signature cache (stored text is immutable).
  mutable size_t textSigHash = 0;
  mutable int textSigLen = 0;
  mutable bool textSigOk = false;

  SingleMessage() {}
  SingleMessage(MessageType mType, QString szUserId, QString szUserName, XmlMessage xmlMessage, QString szId = QString()) {
    type = mType;
    userId = szUserId;
    userName = szUserName;
    message = xmlMessage;
    id = szId;
  }
};

QDataStream& operator<<(QDataStream& out, const SingleMessage& message);
QDataStream& operator>>(QDataStream& in, SingleMessage& message);

class ChatHelper {
public:
  static void makeHtmlSafe(QString* lpszMessage);
  //  Color-emoji fallback families, emoji-first.
  static const QStringList& emojiFallbackFamilies();
  //  Emoji-first with the base family appended last: color emoji when the
  //  color font is installed, its monochrome glyph instead of tofu when not.
  //  Base-first would regress color (the base font usually owns a monochrome
  //  glyph), so the order matters.
  static QStringList emojiFamiliesWithBase(const QString& baseFamily);
  //  Emoji runs get the color font, the rest stays in the base font.
  //  Matched by Unicode ranges, not by the picker list.
  //  isEmojiRun is a potential starter: BMP symbols default to text
  //  presentation (TR51) and need a trailing VS16, which emojiRunLength checks.
  static bool isEmojiRun(uint ucs4);
  static bool isEmojiContinuation(uint ucs4);
  //  UTF-16 length of the run starting at pos, 0 when pos is not a start.
  static int emojiRunLength(const QString& text, int pos);
  //  Wrap emoji runs of escaped HTML in an emoji-font span.
  //  Input must have passed makeHtmlSafe. baseFamily is appended last
  //  (see emojiFamiliesWithBase); empty keeps emoji-only spans.
  static QString wrapEmojiRuns(const QString& safeHtml, const QString& baseFamily = QString());
  static QString emojiSpanStart(const QString& baseFamily = QString());
  static QString emojiSpan(const QString& run, const QString& baseFamily = QString());
  static void mergeFormatOverDocument(QTextEdit* edit, const QTextCharFormat& format);
  static void clearFamiliesOverDocument(QTextEdit* edit);
};
