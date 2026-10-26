// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Direct tests for pure chat units: no model, no delegate, no QTextDocument.

#include <QtTest>

#include "core/definitions.h"
#include "gui/chat/bubblegeometry.h"
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

QTEST_MAIN(TstChatPure)
#include "tst_chatpure.moc"
