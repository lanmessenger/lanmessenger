// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Direct tests for pure chat units: no model, no delegate, no QTextDocument.

#include <QtTest>

#include "core/definitions.h"
#include "gui/chat/bubblegeometry.h"
#include "gui/chat/chathelper.h"
#include "gui/chat/emojipicker.h"
#include "gui/chat/rowhtmlbuilder.h"
#include "gui/chat/rowresolver.h"

namespace {

ResolvedRow makeRow(const QString& card) {
  ResolvedRow r;
  r.card = card;
  return r;
}

Theme::DualColor dual(const QString& light, const QString& dark) {
  Theme::DualColor c;
  c.light = QColor(light);
  c.dark = QColor(dark);
  return c;
}

//  Expected span built from the single source: the fallback list.
//  No hardcoded font names here — they are covered by emojiFamiliesWithBaseOrder.
QString emojiSpan(const QString& run) {
  QString spanStart = QStringLiteral("<span style=\"font-family:'%1'\">")
                        .arg(ChatHelper::emojiFallbackFamilies().join(QStringLiteral("','")));
  return spanStart + run + QStringLiteral("</span>");
}

QString emojiSpanBase(const QString& run, const QString& base) {
  QString spanStart = QStringLiteral("<span style=\"font-family:'%1'\">")
                        .arg(ChatHelper::emojiFamiliesWithBase(base).join(QStringLiteral("','")));
  return spanStart + run + QStringLiteral("</span>");
}

Theme::Bg solidBg(const QString& light, const QString& dark) {
  Theme::Bg bg;
  bg.kind = Theme::Bg::Kind::Solid;
  bg.solid = dual(light, dark);
  return bg;
}

} // namespace

class TstChatPure : public QObject {
  Q_OBJECT

private slots:
  void roleForCard();
  void layoutNameForCard();
  void reservesAvatar();
  void contentWidthEdges();
  void placeBubbleCards();
  void buildLayoutRects();
  void guardedColorDirect();
  void decodeBodyUnicodePassthrough();
  void decodeBodyTrimsEdges();
  void decodeBodyOldCodesStayText();
  void makeHtmlSafeWithEmoji();
  void decodeBodyLinkAroundEmoji();
  void decodeBodyEmojiInsideUrlStaysOut();
  void decodeBodyLineBreaksWithEmoji();
  void emojiFamiliesWithBaseOrder();
  void emojiMatcherStartersAndContinuations();
  void emojiMatcherAllPickerEmojis();
  void emojiRunLengthEdges();
  void wrapEmojiRunsSpans();
};

void TstChatPure::roleForCard() {
  QCOMPARE(RowResolver::roleForCard(MT_Broadcast, false), QStringLiteral("broadcast"));
  QCOMPARE(RowResolver::roleForCard(MT_Broadcast, true), QStringLiteral("broadcast"));
  QCOMPARE(RowResolver::roleForCard(MT_File, false), QStringLiteral("request"));
  QCOMPARE(RowResolver::roleForCard(MT_Folder, true), QStringLiteral("request"));
  QCOMPARE(RowResolver::roleForCard(MT_Failed, false), QStringLiteral("system"));
  QCOMPARE(RowResolver::roleForCard(MT_Join, false), QStringLiteral("system"));
  QCOMPARE(RowResolver::roleForCard(MT_Leave, true), QStringLiteral("system"));
  QCOMPARE(RowResolver::roleForCard(MT_ChatState, false), QStringLiteral("system"));
  QCOMPARE(RowResolver::roleForCard(MT_Message, true), QStringLiteral("outgoing"));
  QCOMPARE(RowResolver::roleForCard(MT_Message, false), QStringLiteral("incoming"));
}

void TstChatPure::layoutNameForCard() {
  QCOMPARE(RowResolver::layoutNameForCard(QStringLiteral("incoming"), false),
           QStringLiteral("layout_incoming"));
  QCOMPARE(RowResolver::layoutNameForCard(QStringLiteral("incoming"), true),
           QStringLiteral("layout_incoming_next"));
  QCOMPARE(RowResolver::layoutNameForCard(QStringLiteral("outgoing"), false),
           QStringLiteral("layout_outgoing"));
  QCOMPARE(RowResolver::layoutNameForCard(QStringLiteral("outgoing"), true),
           QStringLiteral("layout_outgoing_next"));
  QCOMPARE(RowResolver::layoutNameForCard(QStringLiteral("system"), true),
           QStringLiteral("layout_system"));
  QCOMPARE(RowResolver::layoutNameForCard(QStringLiteral("broadcast"), true),
           QStringLiteral("layout_broadcast"));
  QCOMPARE(RowResolver::layoutNameForCard(QStringLiteral("request"), true),
           QStringLiteral("layout_request"));
}

void TstChatPure::reservesAvatar() {
  QVERIFY(RowResolver::reservesAvatar(QStringLiteral("incoming"), 26));
  QVERIFY(!RowResolver::reservesAvatar(QStringLiteral("incoming"), 0));
  QVERIFY(!RowResolver::reservesAvatar(QStringLiteral("outgoing"), 26));
  QVERIFY(!RowResolver::reservesAvatar(QStringLiteral("system"), 26));
  QVERIFY(!RowResolver::reservesAvatar(QStringLiteral("broadcast"), 26));
}

void TstChatPure::contentWidthEdges() {
  ResolvedRow incoming = makeRow(QStringLiteral("incoming"));
  incoming.avatar = true;
  incoming.hboxGap = 8;
  incoming.padding[1] = 14;
  incoming.padding[3] = 14;
  incoming.maxWidthPct = 80;
  //  400 - (26 + 8) = 366, 366 * 80 / 100 = 292, minus 28 padding = 264.
  QCOMPARE(BubbleGeometry::contentWidth(incoming, 400, 26), 264);

  ResolvedRow strip = makeRow(QStringLiteral("broadcast"));
  QCOMPARE(BubbleGeometry::contentWidth(strip, 400, 26), 398);

  ResolvedRow tiny = makeRow(QStringLiteral("incoming"));
  tiny.maxWidthPct = 100;
  QCOMPARE(BubbleGeometry::contentWidth(tiny, 50, 0), 80);

  ResolvedRow full = makeRow(QStringLiteral("outgoing"));
  full.maxWidthPct = 100;
  QCOMPARE(BubbleGeometry::contentWidth(full, 400, 0), 400);
}

void TstChatPure::placeBubbleCards() {
  ResolvedRow outgoing = makeRow(QStringLiteral("outgoing"));
  QRect right = BubbleGeometry::placeBubble(outgoing, 400, 100, 20, 0, 0);
  QCOMPARE(right.right(), 399);
  QCOMPARE(right.left(), 300);

  ResolvedRow wide = makeRow(QStringLiteral("outgoing"));
  QRect clamped = BubbleGeometry::placeBubble(wide, 400, 500, 20, 0, 0);
  QCOMPARE(clamped.left(), 1);

  ResolvedRow system = makeRow(QStringLiteral("system"));
  QRect centered = BubbleGeometry::placeBubble(system, 400, 100, 20, 0, 0);
  QCOMPARE(centered.left(), 150);

  ResolvedRow bc = makeRow(QStringLiteral("broadcast"));
  QRect strip = BubbleGeometry::placeBubble(bc, 400, 100, 20, 0, 0);
  QCOMPARE(strip.left(), 1);

  ResolvedRow incomingAvatar = makeRow(QStringLiteral("incoming"));
  incomingAvatar.avatar = true;
  incomingAvatar.hboxGap = 8;
  QRect withAvatar = BubbleGeometry::placeBubble(incomingAvatar, 400, 100, 20, 0, 26);
  QCOMPARE(withAvatar.left(), 34);
}

void TstChatPure::buildLayoutRects() {
  ResolvedRow row = makeRow(QStringLiteral("incoming"));
  row.padding[0] = 10;
  row.padding[3] = 14;
  QFont name;
  QFont time;
  RowGeometry flat =
    BubbleGeometry::buildLayout(row, 400, 100, 20, name, time, 0);
  QCOMPARE(flat.headerH, 0);
  QCOMPARE(flat.headerAscent, 0);
  QCOMPARE(flat.docRect.left(), flat.bubbleRect.left() + 14);
  QCOMPARE(flat.docRect.top(), flat.bubbleRect.top() + 10);
  QCOMPARE(flat.size.width(), 400);
  QCOMPARE(flat.size.height(), flat.bubbleRect.bottom() + 1);

  ResolvedRow headed = makeRow(QStringLiteral("incoming"));
  headed.header = true;
  headed.vboxGap = 2;
  RowGeometry withHeader =
    BubbleGeometry::buildLayout(headed, 400, 100, 20, name, time, 0);
  QVERIFY(withHeader.headerH > 0);
  QVERIFY(withHeader.headerAscent > 0);
  QCOMPARE(withHeader.bubbleRect.top(), headed.marginTop + withHeader.headerH);
}

void TstChatPure::guardedColorDirect() {
  Theme::DualColor fallback = dual(QStringLiteral("#0F172A"), QStringLiteral("#E2E8F0"));
  Theme::DualColor page = dual(QStringLiteral("#FFFFFF"), QStringLiteral("#1F242B"));
  Theme::Bg white = solidBg(QStringLiteral("#FFFFFF"), QStringLiteral("#1F242B"));

  QColor invalid;
  QCOMPARE(RowHtmlBuilder::guardedColor(invalid, white, fallback, page, false),
           QColor(QStringLiteral("#0F172A")));

  QColor red(QStringLiteral("#FF0000"));
  QCOMPARE(RowHtmlBuilder::guardedColor(red, white, fallback, page, false), red);

  QColor yellow(QStringLiteral("#FFFF00"));
  QCOMPARE(RowHtmlBuilder::guardedColor(yellow, white, fallback, page, false),
           QColor(QStringLiteral("#0F172A")));

  Theme::Bg transparent;
  transparent.kind = Theme::Bg::Kind::Transparent;
  QCOMPARE(RowHtmlBuilder::guardedColor(yellow, transparent, fallback, page, false),
           QColor(QStringLiteral("#0F172A")));
}

void TstChatPure::decodeBodyUnicodePassthrough() {
  QCOMPARE(RowHtmlBuilder::decodeBody(QStringLiteral("😀👍❤️"), false, false, false),
           emojiSpan(QStringLiteral("😀")) + emojiSpan(QStringLiteral("👍")) +
             emojiSpan(QStringLiteral("❤️")));
  QCOMPARE(RowHtmlBuilder::decodeBody(QStringLiteral("hi 😀"), false, false, false),
           QStringLiteral("hi&nbsp;") + emojiSpan(QStringLiteral("😀")));
  //  Bare text-presentation symbols stay in the base font.
  QCOMPARE(RowHtmlBuilder::decodeBody(QStringLiteral("Copyright © 2026"), false, false,
                                      false),
           QStringLiteral("Copyright&nbsp;©&nbsp;2026"));
  QCOMPARE(RowHtmlBuilder::decodeBody(QStringLiteral("hi 😀"), false, false, false,
                                      QStringLiteral("DejaVu Sans")),
           QStringLiteral("hi&nbsp;") +
             emojiSpanBase(QStringLiteral("😀"), QStringLiteral("DejaVu Sans")));
}

void TstChatPure::decodeBodyTrimsEdges() {
  QCOMPARE(RowHtmlBuilder::decodeBody(QStringLiteral("  😀  "), true, false, false),
           emojiSpan(QStringLiteral("😀")));
}

//  Legacy codes from old peers/history stay plain text: no <img>, no mapping.
void TstChatPure::decodeBodyOldCodesStayText() {
  QString out =
    RowHtmlBuilder::decodeBody(QStringLiteral(":-) (H)"), false, false, false);
  QCOMPARE(out, QStringLiteral(":-)&nbsp;(H)"));
  QVERIFY(!out.contains(QStringLiteral("<img")));
  QVERIFY(!out.contains(QStringLiteral(":/smileys")));
}

void TstChatPure::makeHtmlSafeWithEmoji() {
  QString s = QStringLiteral("<b>😀 & \"q\"</b>");
  ChatHelper::makeHtmlSafe(&s);
  QCOMPARE(s, QStringLiteral("&lt;b&gt;😀&nbsp;&amp;&nbsp;&quot;q&quot;&lt;/b&gt;"));

  QString plain = QStringLiteral("😀👍");
  ChatHelper::makeHtmlSafe(&plain);
  QCOMPARE(plain, QStringLiteral("😀👍"));
}

//  Link wrapping must not eat adjacent emoji; emoji must not join the URL.
void TstChatPure::decodeBodyLinkAroundEmoji() {
  QCOMPARE(RowHtmlBuilder::decodeBody(QStringLiteral("see www.x 😀"), false, true,
                                      false),
           QStringLiteral("see&nbsp;<a data-isLink='true' "
                          "href='http://www.x'>www.x</a>&nbsp;") +
             emojiSpan(QStringLiteral("😀")));
}

void TstChatPure::decodeBodyEmojiInsideUrlStaysOut() {
  QCOMPARE(RowHtmlBuilder::decodeBody(QStringLiteral("go www.x/😀/end"), false,
                                      true, false),
           QStringLiteral("go&nbsp;<a data-isLink='true' "
                          "href='http://www.x/'>www.x/</a>") +
             emojiSpan(QStringLiteral("😀")) + QStringLiteral("/end"));
}

void TstChatPure::decodeBodyLineBreaksWithEmoji() {
  QCOMPARE(RowHtmlBuilder::decodeBody(QStringLiteral("a\n😀\n:-)"), false, false,
                                      false),
           QStringLiteral("a<br/>") + emojiSpan(QStringLiteral("😀")) +
             QStringLiteral("<br/>:-)"));
}

void TstChatPure::emojiFamiliesWithBaseOrder() {
  //  Emoji-first, base last; no duplicates.
  QCOMPARE(ChatHelper::emojiFamiliesWithBase(QStringLiteral("DejaVu Sans")),
           QStringList({QStringLiteral("Noto Color Emoji"),
                        QStringLiteral("Segoe UI Emoji"),
                        QStringLiteral("Apple Color Emoji"),
                        QStringLiteral("DejaVu Sans")}));
  QCOMPARE(ChatHelper::emojiFamiliesWithBase(QString()),
           QStringList({QStringLiteral("Noto Color Emoji"),
                        QStringLiteral("Segoe UI Emoji"),
                        QStringLiteral("Apple Color Emoji")}));
  QCOMPARE(ChatHelper::emojiFamiliesWithBase(QStringLiteral("Noto Color Emoji")).size(),
           3);
}

void TstChatPure::emojiMatcherStartersAndContinuations() {
  //  Potential starters: BMP symbols need a trailing VS16 (see edges below).
  QVERIFY(ChatHelper::isEmojiRun(0x1F600)); // 😀
  QVERIFY(ChatHelper::isEmojiRun(0x1F44D)); // 👍
  QVERIFY(ChatHelper::isEmojiRun(0x2764));  // ❤
  QVERIFY(ChatHelper::isEmojiRun(0x2705));  // ✅
  QVERIFY(ChatHelper::isEmojiRun(0x1F525)); // 🔥
  QVERIFY(ChatHelper::isEmojiRun(0x1F389)); // 🎉
  QVERIFY(ChatHelper::isEmojiRun(0x00A9));  // ©
  QVERIFY(ChatHelper::isEmojiRun(0x2B50));  // ⭐
  QVERIFY(!ChatHelper::isEmojiRun(uint(' ')));
  QVERIFY(!ChatHelper::isEmojiRun(uint('A')));
  QVERIFY(!ChatHelper::isEmojiRun(uint(':')));
  QVERIFY(!ChatHelper::isEmojiRun(uint('-')));
  QVERIFY(!ChatHelper::isEmojiRun(uint(')')));
  QVERIFY(!ChatHelper::isEmojiRun(uint('5')));
  //  Modifiers never start a run.
  QVERIFY(!ChatHelper::isEmojiRun(0xFE0F));  // VS16
  QVERIFY(!ChatHelper::isEmojiRun(0x200D));  // ZWJ
  QVERIFY(!ChatHelper::isEmojiRun(0x1F3FD)); // skin tone
  QVERIFY(!ChatHelper::isEmojiRun(0x20E3));  // keycap
  QVERIFY(ChatHelper::isEmojiContinuation(0xFE0F));
  QVERIFY(ChatHelper::isEmojiContinuation(0x200D));
  QVERIFY(ChatHelper::isEmojiContinuation(0x1F3FB));
  QVERIFY(ChatHelper::isEmojiContinuation(0x1F3FF));
  QVERIFY(ChatHelper::isEmojiContinuation(0x20E3));
  QVERIFY(ChatHelper::isEmojiContinuation(0x1F1FA)); // RI U
  QVERIFY(!ChatHelper::isEmojiContinuation(uint(' ')));
  QVERIFY(!ChatHelper::isEmojiContinuation(uint('A')));
}

void TstChatPure::emojiMatcherAllPickerEmojis() {
  //  Every picker cell is exactly one run.
  const QStringList& emojis = EmojiPicker::emojis();
  QCOMPARE(emojis.size(), 80);
  for(const QString& e : emojis) {
    QVERIFY2(!e.isEmpty(), qPrintable(QStringLiteral("empty picker cell")));
    QCOMPARE(ChatHelper::emojiRunLength(e, 0), e.length());
  }
}

void TstChatPure::emojiRunLengthEdges() {
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("abc"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral(":-)"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("5"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("5"), 1), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("1\u20E3"), 0), 2);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("1\uFE0F\u20E3"), 0), 3);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("#\u20E3"), 0), 2);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("12\u20E3"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u2764\uFE0F"), 0), 2);
  //  Text-presentation symbols need VS16; a bare heart/check/arrow/copyright
  //  is base text.
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u2764"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u00A9"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u00A9\uFE0F"), 0), 2);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u2705"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u2705\uFE0F"), 0), 2);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u2194"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u2194\uFE0F"), 0), 2);
  //  Narrowed ranges: neighbors of (c)/(r)/!!/!? never start a run.
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u00AA"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u00AA\uFE0F"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u2044"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\u2044\uFE0F"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\uFE0F"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\U0001F44D\U0001F3FD"), 0),
           4);
  QCOMPARE(ChatHelper::emojiRunLength(
             QStringLiteral("\U0001F984\u200D\U0001F308"), 0),
           5);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\U0001F600\u200D"), 0),
           2);
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\U0001F600\u200Dx"), 0),
           2);
  QCOMPARE(ChatHelper::emojiRunLength(
             QStringLiteral("\U0001F1FA\U0001F1F8"), 0),
           4);
  //  Regional indicators pair into flags; a lone RI is plain text and an odd
  //  tail stays out of the run.
  QCOMPARE(ChatHelper::emojiRunLength(QStringLiteral("\U0001F1E6"), 0), 0);
  QCOMPARE(ChatHelper::emojiRunLength(
             QStringLiteral("\U0001F1FA\U0001F1F8\U0001F1E6"), 0),
           4);
  QCOMPARE(ChatHelper::emojiRunLength(QString(QChar(0xD83D)), 0), 0);
  QString two = QStringLiteral("\U0001F600\U0001F44D");
  QCOMPARE(ChatHelper::emojiRunLength(two, 0), 2);
  QCOMPARE(ChatHelper::emojiRunLength(two, 2), 2);
}

void TstChatPure::wrapEmojiRunsSpans() {
  QCOMPARE(ChatHelper::wrapEmojiRuns(QString()),
           QString());
  QCOMPARE(ChatHelper::wrapEmojiRuns(QStringLiteral("plain")),
           QStringLiteral("plain"));
  QCOMPARE(ChatHelper::wrapEmojiRuns(QStringLiteral(":-)")),
           QStringLiteral(":-)"));
  //  Escapes pass through, emoji gets its span.
  QCOMPARE(ChatHelper::wrapEmojiRuns(QStringLiteral("&lt;\U0001F600&nbsp;&amp;")),
           QStringLiteral("&lt;") + emojiSpan(QStringLiteral("\U0001F600")) +
             QStringLiteral("&nbsp;&amp;"));
  QString rainbow = QStringLiteral("\U0001F984\u200D\U0001F308");
  QCOMPARE(ChatHelper::wrapEmojiRuns(rainbow), emojiSpan(rainbow));
  QCOMPARE(ChatHelper::wrapEmojiRuns(QStringLiteral("1\u20E3")),
           emojiSpan(QStringLiteral("1\u20E3")));
  QCOMPARE(ChatHelper::wrapEmojiRuns(QStringLiteral("5")),
           QStringLiteral("5"));
  //  Text-presentation symbols without VS16 stay in the base font.
  QCOMPARE(ChatHelper::wrapEmojiRuns(QStringLiteral("Copyright © 2026")),
           QStringLiteral("Copyright © 2026"));
  QCOMPARE(ChatHelper::wrapEmojiRuns(QStringLiteral("hi&nbsp;\U0001F600")),
           QStringLiteral("hi&nbsp;") +
             emojiSpan(QStringLiteral("\U0001F600")));
  //  Base family is appended last, after the emoji families.
  QCOMPARE(ChatHelper::wrapEmojiRuns(QStringLiteral("\U0001F600"),
                                     QStringLiteral("DejaVu Sans")),
           emojiSpanBase(QStringLiteral("\U0001F600"), QStringLiteral("DejaVu Sans")));
}

QTEST_MAIN(TstChatPure)
#include "tst_chatpure.moc"
