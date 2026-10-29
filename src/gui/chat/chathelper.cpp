// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QFile>
#include <QDataStream>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextFragment>
#include "chathelper.h"

QDataStream& operator<<(QDataStream& out, const SingleMessage& message) {
  out << qint32(message.type) << message.userId << message.userName << message.message.toString()
      << message.id;
  return out;
}

QDataStream& operator>>(QDataStream& in, SingleMessage& message) {
  qint32 type;
  QString userId;
  QString userName;
  QString xmlMessage;
  QString id;
  in >> type >> userId >> userName >> xmlMessage >> id;
  message = SingleMessage((MessageType)type, userId, userName, XmlMessage(xmlMessage), id);
  return in;
}

void ChatHelper::makeHtmlSafe(QString* lpszMessage) {
  for(int index = 0; index < HTMLESC_COUNT; index++)
    lpszMessage->replace(htmlSymbol[index], htmlEscape[index]);
}

const QStringList& ChatHelper::emojiFallbackFamilies() {
  static const QStringList families = {QStringLiteral("Noto Color Emoji"),
                                       QStringLiteral("Segoe UI Emoji"),
                                       QStringLiteral("Apple Color Emoji")};
  return families;
}

QStringList ChatHelper::emojiFamiliesWithBase(const QString& baseFamily) {
  QStringList families = emojiFallbackFamilies();
  if(!baseFamily.isEmpty() && !families.contains(baseFamily, Qt::CaseInsensitive))
    families.append(baseFamily);
  return families;
}

namespace {

//  Emoji starter ranges. Modifiers are continuations, not starters.
struct EmojiRange {
  uint first;
  uint last;
};

constexpr EmojiRange kEmojiRanges[] = {
  {0x00A9, 0x00A9},   // (c)
  {0x00AE, 0x00AE},   // (r)
  {0x203C, 0x203C},   // !!
  {0x2049, 0x2049},   // !?
  {0x2122, 0x2122},   // (tm)
  {0x2139, 0x2139},   // (i)
  {0x2194, 0x2199},   // arrows
  {0x21A9, 0x21AA},   // hooked arrows
  {0x231A, 0x231B},   // watch/hourglass
  {0x2328, 0x2328},   // keyboard
  {0x2388, 0x2388},   // helm
  {0x23E9, 0x23F3},   // media controls ... hourglass
  {0x23F8, 0x23FA},   // ... record ... nami
  {0x24C2, 0x24C2},   // (m)
  {0x25AA, 0x25AB},   // squares
  {0x25B6, 0x25B6},   // play
  {0x25C0, 0x25C0},   // reverse
  {0x25FB, 0x25FE},   // squares
  {0x2600, 0x26FF},   // misc symbols (U+2764 etc.)
  {0x2700, 0x27BF},   // dingbats (U+2705 etc.)
  {0x2934, 0x2935},   // curved arrows
  {0x2B05, 0x2B07},   // arrows
  {0x2B1B, 0x2B1C},   // squares
  {0x2B50, 0x2B50},   // star
  {0x2B55, 0x2B55},   // circle
  {0x3030, 0x3030},   // wavy dash
  {0x303D, 0x303D},   // part alternation
  {0x3297, 0x3297},   // (congrats)
  {0x3299, 0x3299},   // (secret)
  {0x1F000, 0x1FAFF}, // supplementary emoji: emoticons,
                      // pictographs, transport, supplemental,
                      // chess, extended-A (U+1F600, U+1F44D etc.)
  {0x1FC00, 0x1FFFD}, // legacy computing symbols
};

constexpr uint kVs16 = 0xFE0F;
constexpr uint kZwj = 0x200D;
constexpr uint kKeycap = 0x20E3;
constexpr uint kSkinFirst = 0x1F3FB;
constexpr uint kSkinLast = 0x1F3FF;
constexpr uint kRiFirst = 0x1F1E6;
constexpr uint kRiLast = 0x1F1FF;

uint codePointAt(const QString& text, int pos, int* utf16Len) {
  QChar high = text.at(pos);
  if(high.isHighSurrogate() && pos + 1 < text.length()) {
    QChar low = text.at(pos + 1);
    if(low.isLowSurrogate()) {
      if(utf16Len)
        *utf16Len = 2;
      return QChar::surrogateToUcs4(high, low);
    }
  }
  if(utf16Len)
    *utf16Len = 1;
  //  Lone surrogate: return the unit.
  return high.unicode();
}

bool isKeycapBase(uint ucs4) {
  return (ucs4 >= 0x0030 && ucs4 <= 0x0039) || ucs4 == 0x0023 || ucs4 == 0x002A;
}

bool isRegionalIndicator(uint ucs4) {
  return ucs4 >= kRiFirst && ucs4 <= kRiLast;
}

//  BMP symbols default to text presentation (TR51) and need a trailing VS16
//  to count as emoji; supplementary starters default to emoji.
bool emojiNeedsVs16(uint ucs4) {
  return ucs4 < 0x1F000;
}

} // namespace

bool ChatHelper::isEmojiRun(uint ucs4) {
  if(ucs4 == kVs16 || ucs4 == kZwj || ucs4 == kKeycap)
    return false;
  if(ucs4 >= kSkinFirst && ucs4 <= kSkinLast)
    return false;
  if(isKeycapBase(ucs4))
    return false;
  for(const EmojiRange& r : kEmojiRanges)
    if(ucs4 >= r.first && ucs4 <= r.last)
      return true;
  return false;
}

bool ChatHelper::isEmojiContinuation(uint ucs4) {
  if(ucs4 == kVs16 || ucs4 == kZwj || ucs4 == kKeycap)
    return true;
  if(ucs4 >= kSkinFirst && ucs4 <= kSkinLast)
    return true;
  if(ucs4 >= kRiFirst && ucs4 <= kRiLast)
    return true;
  return false;
}

int ChatHelper::emojiRunLength(const QString& text, int pos) {
  if(pos < 0 || pos >= text.length())
    return 0;
  int len = 0;
  uint cp = codePointAt(text, pos, &len);
  //  Keycap: digit/#/* + U+20E3.
  if(isKeycapBase(cp)) {
    int cur = pos + len;
    int total = len;
    if(cur < text.length()) {
      int step = 0;
      uint next = codePointAt(text, cur, &step);
      if(next == kVs16) {
        cur += step;
        total += step;
        if(cur >= text.length())
          return 0;
        next = codePointAt(text, cur, &step);
      }
      if(next == kKeycap)
        return total + step;
    }
    return 0;
  }
  if(!isEmojiRun(cp))
    return 0;
  const bool needsVs16 = emojiNeedsVs16(cp);
  int total = len;
  int cur = pos + len;
  //  Regional indicators pair into flags; a lone RI is plain text.
  if(isRegionalIndicator(cp)) {
    int riEnd = cur;
    int riUnits = len;
    while(riEnd < text.length()) {
      int step = 0;
      uint next = codePointAt(text, riEnd, &step);
      if(!isRegionalIndicator(next))
        break;
      riEnd += step;
      riUnits += step;
    }
    //  RI are supplementary: one code point is 2 UTF-16 units.
    const int riCount = riUnits / 2;
    if(riCount < 2)
      return 0;
    const int paired = (riCount & ~1) * 2;
    cur = pos + paired;
    total = paired;
  }
  //  Trailing modifiers join the run; ZWJ only with a starter after it.
  bool sawVs16 = false;
  while(cur < text.length()) {
    int step = 0;
    uint next = codePointAt(text, cur, &step);
    if(next == kZwj) {
      int after = 0;
      if(cur + step >= text.length())
        break;
      uint joined = codePointAt(text, cur + step, &after);
      if(!isEmojiRun(joined) && !isKeycapBase(joined))
        break;
      if(isKeycapBase(joined)) {
        int keyLen = emojiRunLength(text, cur + step);
        if(keyLen == 0)
          break;
        cur += step + keyLen;
        total += step + keyLen;
        continue;
      }
      cur += step + after;
      total += step + after;
      while(isRegionalIndicator(joined) && cur < text.length()) {
        int second = 0;
        uint b = codePointAt(text, cur, &second);
        if(!isRegionalIndicator(b))
          break;
        cur += second;
        total += second;
      }
      continue;
    }
    //  Plain modifiers come from the single continuation set; ZWJ joins
    //  above and RI only pair into flags, so both stay out here.
    if(isEmojiContinuation(next) && next != kZwj && !isRegionalIndicator(next)) {
      if(next == kVs16)
        sawVs16 = true;
      cur += step;
      total += step;
      continue;
    }
    break;
  }
  //  Text-presentation symbols (all BMP starters) only count with VS16.
  if(needsVs16 && !sawVs16)
    return 0;
  return total;
}

QString ChatHelper::emojiSpanStart(const QString& baseFamily) {
  return QStringLiteral("<span style=\"font-family:'%1'\">")
    .arg(emojiFamiliesWithBase(baseFamily).join(QStringLiteral("','")));
}

QString ChatHelper::emojiSpan(const QString& run, const QString& baseFamily) {
  return emojiSpanStart(baseFamily) + run + QStringLiteral("</span>");
}

QString ChatHelper::wrapEmojiRuns(const QString& safeHtml, const QString& baseFamily) {
  QString out;
  out.reserve(safeHtml.length() + 32);
  int pos = 0;
  while(pos < safeHtml.length()) {
    int run = emojiRunLength(safeHtml, pos);
    if(run > 0) {
      out += emojiSpan(safeHtml.mid(pos, run), baseFamily);
      pos += run;
    } else {
      //  Don't split surrogate pairs.
      QChar c = safeHtml.at(pos);
      if(c.isHighSurrogate() && pos + 1 < safeHtml.length() &&
         safeHtml.at(pos + 1).isLowSurrogate()) {
        out += safeHtml.mid(pos, 2);
        pos += 2;
      } else {
        out += c;
        pos += 1;
      }
    }
  }
  return out;
}

QTextBlockData::QTextBlockData(QString id) {
  this->id = id;
}

QTextBlockData::~QTextBlockData() {
}

void ChatHelper::mergeFormatOverDocument(QTextEdit* edit, const QTextCharFormat& format) {
  QTextCursor saved = edit->textCursor();
  int anchor = saved.anchor();
  int position = saved.position();
  QTextCursor cursor(edit->document());
  cursor.select(QTextCursor::Document);
  cursor.mergeCharFormat(format);
  QTextCursor restore(edit->document());
  restore.setPosition(anchor);
  restore.setPosition(position, QTextCursor::KeepAnchor);
  edit->setTextCursor(restore);
}

void ChatHelper::clearFamiliesOverDocument(QTextEdit* edit) {
  QTextDocument* doc = edit->document();
  for(QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
    for(QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
      const QTextFragment fragment = it.fragment();
      if(!fragment.isValid() || fragment.length() <= 0)
        continue;
      QTextCharFormat format = fragment.charFormat();
      if(!format.hasProperty(QTextFormat::FontFamilies))
        continue;
      format.clearProperty(QTextFormat::FontFamilies);
      QTextCursor cursor(doc);
      cursor.setPosition(fragment.position());
      cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
      cursor.setCharFormat(format);
    }
  }
  QTextCharFormat current = edit->currentCharFormat();
  if(current.hasProperty(QTextFormat::FontFamilies)) {
    current.clearProperty(QTextFormat::FontFamilies);
    edit->setCurrentCharFormat(current);
  }
}
