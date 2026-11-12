// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Header typography in BubbleDelegate: theme + local display size only,
// offscreen. The getters take no model index, so the sender-chosen message
// font cannot leak into name/time by construction.

#include <QtTest>

#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>

#include <functional>

#include "core/definitions.h"
#include "core/theme.h"
#include "gui/chat/bubbledelegate.h"
#include "gui/chat/chatdefinitions.h"
#include "gui/chat/chatmessagemodel.h"
#include "messaging/xmlmessage.h"

namespace {

QByteArray meshBlueBytes() {
  QFile f(QStringLiteral(":/themes/MeshBlue.json"));
  if(!f.open(QIODevice::ReadOnly))
    return QByteArray();
  return f.readAll();
}

Theme scaledTheme(double senderSize, double timeSize) {
  QJsonDocument doc = QJsonDocument::fromJson(meshBlueBytes());
  QJsonObject root = doc.object();
  QJsonObject h = root.value(QStringLiteral("header")).toObject();
  h.insert(QStringLiteral("senderSize"), senderSize);
  h.insert(QStringLiteral("timeSize"), timeSize);
  root.insert(QStringLiteral("header"), h);
  doc.setObject(root);
  return Theme::loadFromJson(doc.toJson());
}

} // namespace

class TstBubbleHeader : public QObject {
  Q_OBJECT

private slots:
  void headerUsesThemeAndLocalSize();
  void headerScalesWithTheme();
  void headerFollowsLocalSize();
  void shippedHeaderColorsResolve();
  void seriesTopMargins();
  void seriesCornerRadii();
  void outgoingCornersReadVerbatim();
  void layoutResolvesRow();
  void resizeRecomputesHeight();
  void broadcastRectClearsViewportEdge();
  void paintRowsOffscreen();
  void dateStripSkipsTimelessAndTyping();
  void dateStripFallbackMirrorsModel();
};

XmlMessage textMessage(const QString& text) {
  XmlMessage m;
  m.addHeader(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
  m.addData(XN_MESSAGE, text);
  m.addData(XN_FONT, QFont().toString());
  m.addData(XN_COLOR, QStringLiteral("#000000"));
  return m;
}

XmlMessage textMessageNoTime(const QString& text) {
  XmlMessage m;
  m.addData(XN_MESSAGE, text);
  m.addData(XN_FONT, QFont().toString());
  m.addData(XN_COLOR, QStringLiteral("#000000"));
  return m;
}

void TstBubbleHeader::headerUsesThemeAndLocalSize() {
  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  BubbleDelegate d;
  d.setTheme(t);
  //  Default display settings: fontSizeVal=1 -> 9.0pt, system family.
  QFont name = d.headerNameFont();
  QFont time = d.headerTimeFont();
  QCOMPARE(name.pointSizeF(), 9.0);
  QCOMPARE(time.pointSizeF(), 9.0);
  QCOMPARE(name.family(), QFont().family());
  QCOMPARE(time.family(), QFont().family());
  QCOMPARE(name.weight(), QFont::DemiBold);
  QCOMPARE(time.weight(), QFont::Normal);
}

void TstBubbleHeader::headerScalesWithTheme() {
  Theme t = scaledTheme(1.25, 0.75);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  BubbleDelegate d;
  d.setTheme(t);
  QCOMPARE(d.headerNameFont().pointSizeF(), 9.0 * 1.25);
  QCOMPARE(d.headerTimeFont().pointSizeF(), 9.0 * 0.75);
}

void TstBubbleHeader::headerFollowsLocalSize() {
  Theme t = scaledTheme(1.0, 1.0);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  BubbleDelegate d;
  d.setTheme(t);
  BubbleDelegate::DisplaySettings ds;
  ds.fontSizeVal = 0;
  d.setDisplaySettings(ds);
  QCOMPARE(d.headerNameFont().pointSizeF(), 8.25);
  ds.fontSizeVal = 2;
  d.setDisplaySettings(ds);
  QCOMPARE(d.headerTimeFont().pointSizeF(), 10.5);
}

void TstBubbleHeader::shippedHeaderColorsResolve() {
  //  Header colors are literals now: both modes resolve on every theme.
  //  Links live per rect: every layout rect carries a linkColor for both modes.
  const char* names[] = {"ClassicSlate", "MeshBlue", "NordicTerminal"};
  const char* layouts[] = {"layout_incoming", "layout_incoming_next", "layout_outgoing",
                           "layout_outgoing_next", "layout_system", "layout_broadcast",
                           "layout_request"};
  for(const char* n : names) {
    QFile f(QString::fromLatin1(":/themes/%1.json").arg(QString::fromLatin1(n)));
    QVERIFY(f.open(QIODevice::ReadOnly));
    Theme t = Theme::loadFromJson(f.readAll());
    QVERIFY2(t.isValid, qPrintable(t.errorString));
    QVERIFY(Theme::pick(t.header.senderColor, false).isValid());
    QVERIFY(Theme::pick(t.header.senderColor, true).isValid());
    QVERIFY(Theme::pick(t.header.timeColor, false).isValid());
    QVERIFY(Theme::pick(t.header.timeColor, true).isValid());
    QVERIFY(Theme::pick(t.text, false).isValid());
    QVERIFY(Theme::pick(t.page, false).isValid());
    for(const char* layout : layouts) {
      const Theme::LayoutNode& root = t.compiledLayout(QString::fromLatin1(layout));
      const Theme::LayoutNode* rect = nullptr;
      std::function<void(const Theme::LayoutNode&)> find = [&](const Theme::LayoutNode& node) {
        if(node.type == Theme::LayoutNode::Type::Body ||
           node.type == Theme::LayoutNode::Type::FileCard)
          rect = &node;
        for(const Theme::LayoutNode& c : node.children)
          find(c);
      };
      find(root);
      QVERIFY2(rect != nullptr, qPrintable(QString::fromLatin1("%1 %2 without rect").arg(n, layout)));
      QVERIFY2(rect->hasLinkColor,
               qPrintable(QString::fromLatin1("%1 %2 without linkColor").arg(n, layout)));
      QVERIFY(Theme::pick(rect->linkColor, false).isValid());
      QVERIFY(Theme::pick(rect->linkColor, true).isValid());
    }
  }
}

void TstBubbleHeader::seriesTopMargins() {
  //  Series of 3 + 1 other: inside the series the dense marginTop, outside
  //  the series-start marginTop. MeshBlue ships marginTop 9 on
  //  series-start trees, 3 on the _next trees.
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  model.localId = me;
  for(const QString& text : {QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three")}) {
    XmlMessage m = textMessage(text);
    model.appendMessageLog(MT_Message, &jack, &jackName, &m);
  }
  XmlMessage m4 = textMessage(QStringLiteral("four"));
  model.appendMessageLog(MT_Message, &me, &meName, &m4);

  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  BubbleDelegate d;
  d.setTheme(t);
  QCOMPARE(d.resolveRow(model.index(0, 0)).marginTop, 9);
  QCOMPARE(d.resolveRow(model.index(1, 0)).marginTop, 3);
  QCOMPARE(d.resolveRow(model.index(2, 0)).marginTop, 3);
  QCOMPARE(d.resolveRow(model.index(3, 0)).marginTop, 9);

  //  Same text + same grouped flag => identical row height; the series
  //  start (top margin + header) is taller than an inner row.
  QStyleOptionViewItem option;
  option.rect = QRect(0, 0, 400, 10);
  ChatMessageModel model2;
  model2.localId = me;
  for(int i = 0; i < 3; i++) {
    XmlMessage same = textMessage(QStringLiteral("same"));
    model2.appendMessageLog(MT_Message, &jack, &jackName, &same);
  }
  BubbleDelegate d2;
  d2.setTheme(t);
  QCOMPARE(d2.sizeHint(option, model2.index(1, 0)).height(),
           d2.sizeHint(option, model2.index(2, 0)).height());
  QVERIFY(d2.sizeHint(option, model2.index(0, 0)).height() >
          d2.sizeHint(option, model2.index(1, 0)).height());
}

void TstBubbleHeader::seriesCornerRadii() {
  //  Same series shape as seriesTopMargins; MeshBlue ships corners on the
  //  body nodes: series-start [14, 14, 14, 3], in-series [3, 14, 14, 3]
  //  (window side flattened); outgoing nodes carry hand-mirrored values.
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  model.localId = me;
  for(const QString& text : {QStringLiteral("one"), QStringLiteral("two"), QStringLiteral("three")}) {
    XmlMessage m = textMessage(text);
    model.appendMessageLog(MT_Message, &jack, &jackName, &m);
  }
  XmlMessage m4 = textMessage(QStringLiteral("four"));
  model.appendMessageLog(MT_Message, &me, &meName, &m4);
  XmlMessage m5 = textMessage(QStringLiteral("five"));
  model.appendMessageLog(MT_Message, &me, &meName, &m5);

  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  BubbleDelegate d;
  d.setTheme(t);
  BubbleDelegate::ResolvedRow first = d.resolveRow(model.index(0, 0));
  BubbleDelegate::ResolvedRow next = d.resolveRow(model.index(1, 0));
  BubbleDelegate::ResolvedRow single = d.resolveRow(model.index(3, 0));
  BubbleDelegate::ResolvedRow ownNext = d.resolveRow(model.index(4, 0));
  const int wantBase[4] = {14, 14, 14, 3};
  const int wantNext[4] = {3, 14, 14, 3};
  //  Own rows mirror the base too: tail side is right.
  const int wantOwnSingle[4] = {14, 14, 3, 14};
  const int wantOwnNext[4] = {14, 3, 3, 14};
  for(int i = 0; i < 4; i++) {
    QCOMPARE(first.corners[i], wantBase[i]);
    QCOMPARE(next.corners[i], wantNext[i]);
    QCOMPARE(single.corners[i], wantOwnSingle[i]);
    QCOMPARE(ownNext.corners[i], wantOwnNext[i]);
  }
  //  Third row is grouped too: same radii as the second.
  BubbleDelegate::ResolvedRow third = d.resolveRow(model.index(2, 0));
  for(int i = 0; i < 4; i++)
    QCOMPARE(third.corners[i], wantNext[i]);
}

void TstBubbleHeader::outgoingCornersReadVerbatim() {
  //  No code mirror exists: outgoing corners render exactly as written,
  //  even when they are not the mirror of incoming. The in-series tree
  //  carries its own scalar corners.
  QJsonDocument doc = QJsonDocument::fromJson(meshBlueBytes());
  QJsonObject root = doc.object();
  auto setBodyCorners = [&](const QString& layout, const QJsonArray& corners) {
    //  Roots vary: hbox (incoming) holds the vbox column, vbox roots
    //  (outgoing, in-series) hold the rects directly.
    auto paintColumn = [&](QJsonObject column) {
      QJsonArray kids = column.value(QStringLiteral("children")).toArray();
      for(int j = 0; j < kids.size(); j++) {
        QJsonObject node = kids.at(j).toObject();
        if(node.value(QStringLiteral("type")).toString() != QStringLiteral("body"))
          continue;
        node.insert(QStringLiteral("corners"), corners);
        kids.replace(j, node);
      }
      column.insert(QStringLiteral("children"), kids);
      return column;
    };
    QJsonObject node = root.value(layout).toObject();
    if(node.value(QStringLiteral("type")).toString() == QStringLiteral("hbox")) {
      QJsonArray hboxKids = node.value(QStringLiteral("children")).toArray();
      for(int i = 0; i < hboxKids.size(); i++) {
        QJsonObject kid = hboxKids.at(i).toObject();
        if(kid.value(QStringLiteral("type")).toString() != QStringLiteral("vbox"))
          continue;
        hboxKids.replace(i, paintColumn(kid));
      }
      node.insert(QStringLiteral("children"), hboxKids);
    } else {
      node = paintColumn(node);
    }
    root.insert(layout, node);
  };
  QJsonArray singleCorners;
  singleCorners.append(1);
  singleCorners.append(2);
  singleCorners.append(3);
  singleCorners.append(4);
  QJsonArray nextCorners;
  nextCorners.append(5);
  nextCorners.append(6);
  nextCorners.append(7);
  nextCorners.append(8);
  setBodyCorners(QStringLiteral("layout_outgoing"), singleCorners);
  setBodyCorners(QStringLiteral("layout_outgoing_next"), nextCorners);
  doc.setObject(root);
  Theme t = Theme::loadFromJson(doc.toJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));

  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  model.localId = me;
  XmlMessage m1 = textMessage(QStringLiteral("one"));
  model.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  for(const QString& text : {QStringLiteral("two"), QStringLiteral("three")}) {
    XmlMessage m = textMessage(text);
    model.appendMessageLog(MT_Message, &me, &meName, &m);
  }
  BubbleDelegate d;
  d.setTheme(t);
  BubbleDelegate::ResolvedRow single = d.resolveRow(model.index(1, 0));
  BubbleDelegate::ResolvedRow next = d.resolveRow(model.index(2, 0));
  const int wantSingle[4] = {1, 2, 3, 4};
  const int wantNext[4] = {5, 6, 7, 8};
  for(int i = 0; i < 4; i++) {
    QCOMPARE(single.corners[i], wantSingle[i]);
    QCOMPARE(next.corners[i], wantNext[i]);
  }
}

void TstBubbleHeader::layoutResolvesRow() {
  //  Same series shape; MeshBlue nodes carry gaps 8/2, literal senderColor
  //  pairs and gradient fills with hand-mirrored corners.
  const QColor textLight(QStringLiteral("#0F172A"));
  const QColor textDark(QStringLiteral("#E2E8F0"));
  const QColor secondaryLight(QStringLiteral("#64748B"));
  const QColor secondaryDark(QStringLiteral("#7DD3FC"));
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  model.localId = me;
  for(const QString& text : {QStringLiteral("one"), QStringLiteral("two")}) {
    XmlMessage m = textMessage(text);
    model.appendMessageLog(MT_Message, &jack, &jackName, &m);
  }
  XmlMessage m3 = textMessage(QStringLiteral("three"));
  model.appendMessageLog(MT_Message, &me, &meName, &m3);

  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  BubbleDelegate d;
  d.setTheme(t);

  BubbleDelegate::ResolvedRow first = d.resolveRow(model.index(0, 0));
  QCOMPARE(first.card, QStringLiteral("incoming"));
  QVERIFY(first.avatar);
  QVERIFY(first.header);
  QCOMPARE(first.headerItems, QStringList({QStringLiteral("sender"), QStringLiteral("time")}));
  QCOMPARE(first.hboxGap, 8);
  QCOMPARE(first.vboxGap, 2);
  //  Node sender wins; time and body text fall back to theme defaults.
  QCOMPARE(first.senderColor.light, textLight);
  QCOMPARE(first.senderColor.dark, textDark);
  QCOMPARE(first.timeColor.light, secondaryLight);
  QCOMPARE(first.timeColor.dark, secondaryDark);
  QCOMPARE(first.text.light, textLight);
  QCOMPARE(first.text.dark, textDark);
  //  Incoming gradient verbatim from the node, metrics ride the rect.
  QCOMPARE(first.bg.kind, Theme::Bg::Kind::Gradient);
  QCOMPARE(first.bg.lightFrom, QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(first.bg.lightTo, QColor(QStringLiteral("#F0FDF4")));
  QCOMPARE(first.bg.darkFrom, QColor(QStringLiteral("#162235")));
  QCOMPARE(first.bg.darkTo, QColor(QStringLiteral("#111A29")));
  QCOMPARE(first.padding[0], 10);
  QCOMPARE(first.padding[1], 14);
  QCOMPARE(first.padding[2], 10);
  QCOMPARE(first.padding[3], 14);
  QCOMPARE(first.shadowDx, 0.0);
  QCOMPARE(first.shadowDy, 0.0);
  QCOMPARE(first.shadowOpacity, 0.0);
  QCOMPARE(first.marginTop, 9);
  QCOMPARE(first.maxWidthPct, 80);
  //  Shipped incoming carries the spec 1px border, no shadow.
  QVERIFY(first.hasBorder);
  QCOMPARE(first.border.light, QColor(QStringLiteral("#E2E8F0")));
  QCOMPARE(first.border.dark, QColor(QStringLiteral("#1E3A5F")));

  BubbleDelegate::ResolvedRow next = d.resolveRow(model.index(1, 0));
  //  In-series rows walk layout_incoming_next: no avatar, no header.
  QVERIFY(!next.avatar);
  QVERIFY(!next.header);
  QVERIFY(next.headerItems.isEmpty());
  QCOMPARE(next.marginTop, 3);

  BubbleDelegate::ResolvedRow own = d.resolveRow(model.index(2, 0));
  QCOMPARE(own.card, QStringLiteral("outgoing"));
  QVERIFY(!own.avatar);
  QVERIFY(own.header);
  QCOMPARE(own.headerItems, QStringList({QStringLiteral("time")}));
  QCOMPARE(own.hboxGap, 8);
  QCOMPARE(own.vboxGap, 2);
  QCOMPARE(own.senderColor.light, textLight);
  //  Saturated outgoing bubble: bare-white text, own gradient.
  QCOMPARE(own.text.light, QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(own.text.dark, QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(own.bg.kind, Theme::Bg::Kind::Gradient);
  QCOMPARE(own.bg.lightFrom, QColor(QStringLiteral("#0284C7")));
  QCOMPARE(own.bg.lightTo, QColor(QStringLiteral("#0369A1")));
  QCOMPARE(own.bg.darkFrom, QColor(QStringLiteral("#0EA5E9")));
  QCOMPARE(own.bg.darkTo, QColor(QStringLiteral("#0284C7")));

  //  Node gaps flow through: vbox gap 5 resolves into the row.
  QJsonDocument doc = QJsonDocument::fromJson(meshBlueBytes());
  QJsonObject root = doc.object();
  QJsonObject hbox = root.value(QStringLiteral("layout_incoming")).toObject();
  QJsonArray hboxKids = hbox.value(QStringLiteral("children")).toArray();
  QJsonObject vbox = hboxKids.at(1).toObject();
  vbox.insert(QStringLiteral("gap"), 5);
  hboxKids.replace(1, vbox);
  hbox.insert(QStringLiteral("children"), hboxKids);
  root.insert(QStringLiteral("layout_incoming"), hbox);
  doc.setObject(root);
  Theme t2 = Theme::loadFromJson(doc.toJson());
  QVERIFY2(t2.isValid, qPrintable(t2.errorString));
  BubbleDelegate d2;
  d2.setTheme(t2);
  QCOMPARE(d2.resolveRow(model.index(0, 0)).vboxGap, 5);
}

void TstBubbleHeader::resizeRecomputesHeight() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QString me = QStringLiteral("me");
  model.localId = me;
  QString longText;
  for(int i = 0; i < 40; i++)
    longText += QStringLiteral("word%1 ").arg(i);
  XmlMessage m = textMessage(longText);
  model.appendMessageLog(MT_Message, &jack, &jackName, &m);

  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  BubbleDelegate d;
  d.setTheme(t);
  QStyleOptionViewItem narrow;
  narrow.rect = QRect(0, 0, 240, 10);
  QStyleOptionViewItem wide;
  wide.rect = QRect(0, 0, 640, 10);
  const int narrowH = d.sizeHint(narrow, model.index(0, 0)).height();
  const int wideH = d.sizeHint(wide, model.index(0, 0)).height();
  QVERIFY(narrowH > wideH);
  QCOMPARE(d.sizeHint(narrow, model.index(0, 0)).height(), narrowH);
  d.noteViewportWidth(640);
  QCOMPARE(d.sizeHint(narrow, model.index(0, 0)).height(), narrowH);
}

void TstBubbleHeader::broadcastRectClearsViewportEdge() {
  //  The 1px border stroke is centered on the bubble path: an edge flush
  //  with the viewport loses its outer half to clipping, so the broadcast
  //  left edge rendered thinner than the other three sides. Strip cards
  //  (broadcast/system) keep symmetric 1px gutters; led cards never touch
  //  the edge and keep their geometry.
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  model.localId = me;
  XmlMessage mIn = textMessage(QStringLiteral("hello"));
  model.appendMessageLog(MT_Message, &jack, &jackName, &mIn);
  XmlMessage mBc = textMessage(QStringLiteral("heads up"));
  model.appendMessageLog(MT_Broadcast, &jack, &jackName, &mBc);
  XmlMessage mSys = textMessage(QStringLiteral("oops"));
  model.appendMessageLog(MT_Failed, &jack, &jackName, &mSys);
  XmlMessage mOut = textMessage(QStringLiteral("reply"));
  model.appendMessageLog(MT_Message, &me, &meName, &mOut);

  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  BubbleDelegate d;
  d.setTheme(t);
  const int W = 400;
  //  MeshBlue: avatar 26 + hbox gap 8, incoming padding 14/14,
  //  maxWidthPct 80 -> bubble [34, 325].
  QRect incoming = d.rowBubbleRect(W, 20, d.resolveRow(model.index(0, 0)));
  QCOMPARE(incoming.left(), 34);
  QCOMPARE(incoming.right(), 325);
  //  Strip cards: symmetric 1px gutters, full stroke on all sides.
  QRect bc = d.rowBubbleRect(W, 20, d.resolveRow(model.index(1, 0)));
  QCOMPARE(bc.left(), 1);
  QCOMPARE(bc.right(), W - 2);
  QRect sys = d.rowBubbleRect(W, 20, d.resolveRow(model.index(2, 0)));
  QCOMPARE(sys.left(), 1);
  QCOMPARE(sys.right(), W - 2);
  //  Outgoing keeps its right-flush geometry (that edge renders fully).
  QRect own = d.rowBubbleRect(W, 20, d.resolveRow(model.index(3, 0)));
  QCOMPARE(own.left(), 80);
  QCOMPARE(own.right(), W - 1);
}

void TstBubbleHeader::paintRowsOffscreen() {
  //  End-to-end render smoke: every role paints without crashing, and the
  //  mode actually changes the pixels (literal colors resolve per mode).
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  model.localId = me;
  XmlMessage m1 = textMessage(QStringLiteral("one"));
  model.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  XmlMessage m2 = textMessage(QStringLiteral("two"));
  model.appendMessageLog(MT_Message, &me, &meName, &m2);
  XmlMessage m3 = textMessage(QStringLiteral("sys"));
  model.appendMessageLog(MT_Failed, &jack, &jackName, &m3);
  XmlMessage m4 = textMessage(QStringLiteral("bc"));
  model.appendMessageLog(MT_Broadcast, &jack, &jackName, &m4);
  XmlMessage m5;
  m5.addHeader(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
  m5.addData(XN_FILEID, QStringLiteral("id1"));
  m5.addData(XN_FILEOP, FileOpNames[FO_Request]);
  m5.addData(XN_FILENAME, QStringLiteral("f.bin"));
  m5.addData(XN_FILESIZE, QStringLiteral("800000"));
  m5.addData(XN_FILEPATH, QStringLiteral("/tmp/f.bin"));
  model.appendMessageLog(MT_File, &me, &meName, &m5);

  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QImage light, dark;
  for(int pass = 0; pass < 2; pass++) {
    BubbleDelegate d;
    d.setTheme(t);
    d.setMode(pass == 0 ? Theme::Mode::Light : Theme::Mode::Dark, false);
    int total = 0;
    for(int row = 0; row < model.rowCount(); row++) {
      QStyleOptionViewItem option;
      option.rect = QRect(0, 0, 400, 10);
      QSize size = d.sizeHint(option, model.index(row, 0));
      QVERIFY(size.height() > 0);
      total += size.height();
    }
    QImage img(400, total, QImage::Format_ARGB32);
    img.fill(Qt::magenta); // any unpainted gap screams
    QPainter p(&img);
    int y = 0;
    for(int row = 0; row < model.rowCount(); row++) {
      QStyleOptionViewItem option;
      option.rect = QRect(0, y, 400, 10);
      QSize size = d.sizeHint(option, model.index(row, 0));
      option.rect.setHeight(size.height());
      d.paint(&p, option, model.index(row, 0));
      y += size.height();
    }
    p.end();
    if(pass == 0)
      light = img;
    else
      dark = img;
  }
  QVERIFY(light != dark);
}

void TstBubbleHeader::dateStripSkipsTimelessAndTyping() {
  //  Delegate trusts DateLabelRole: timeless row0 and typing rows never
  //  grow a date strip, even when their TimeRole would look like a new
  //  day (epoch 1970 for missing XN_TIME, next-day stamp for typing).
  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QStyleOptionViewItem option;
  option.rect = QRect(0, 0, 400, 10);

  ChatMessageModel timeless;
  XmlMessage l1 = textMessageNoTime(QStringLiteral("same"));
  XmlMessage l2 = textMessageNoTime(QStringLiteral("same"));
  timeless.appendMessageLog(MT_Message, &jack, &jackName, &l1);
  timeless.appendMessageLog(MT_Message, &jack, &jackName, &l2);
  QVERIFY(timeless.data(timeless.index(0, 0), ChatMessageModel::DateLabelRole)
            .toString()
            .isEmpty());
  QVERIFY(timeless.data(timeless.index(1, 0), ChatMessageModel::DateLabelRole)
            .toString()
            .isEmpty());

  ChatMessageModel dated;
  const qint64 d1 =
    QDateTime(QDate(2024, 1, 10), QTime(12, 0)).toMSecsSinceEpoch();
  XmlMessage m1;
  m1.addHeader(XN_TIME, QString::number(d1));
  m1.addData(XN_MESSAGE, QStringLiteral("same"));
  m1.addData(XN_FONT, QFont().toString());
  m1.addData(XN_COLOR, QStringLiteral("#000000"));
  XmlMessage m2 = m1;
  dated.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  dated.appendMessageLog(MT_Message, &jack, &jackName, &m2);
  QVERIFY(!dated.data(dated.index(0, 0), ChatMessageModel::DateLabelRole)
             .toString()
             .isEmpty());
  QVERIFY(dated.data(dated.index(1, 0), ChatMessageModel::DateLabelRole)
            .toString()
            .isEmpty());
  BubbleDelegate d;
  d.setTheme(t);
  //  Starter + strip is taller than starter without strip; grouped rows
  //  without strip match each other regardless of timeless/dated.
  QVERIFY(d.sizeHint(option, dated.index(0, 0)).height() >
          d.sizeHint(option, timeless.index(0, 0)).height());
  QCOMPARE(d.sizeHint(option, dated.index(1, 0)).height(),
           d.sizeHint(option, timeless.index(1, 0)).height());

  ChatMessageModel withTyping;
  withTyping.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  XmlMessage typing;
  typing.addHeader(XN_TIME, QString::number(d1 + 24 * 60 * 60 * 1000));
  typing.addData(XN_CHATSTATE, ChatStateNames[CS_Composing]);
  withTyping.appendMessageLog(MT_ChatState, &jack, &jackName, &typing);
  QCOMPARE(withTyping.rowCount(), 2);
  QVERIFY(withTyping.isTypingRow(1));
  QVERIFY(withTyping.data(withTyping.index(1, 0), ChatMessageModel::DateLabelRole)
            .toString()
            .isEmpty());
  ChatMessageModel sameDayTyping;
  sameDayTyping.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  XmlMessage typingSameDay;
  typingSameDay.addHeader(XN_TIME, QString::number(d1 + 60 * 1000));
  typingSameDay.addData(XN_CHATSTATE, ChatStateNames[CS_Composing]);
  sameDayTyping.appendMessageLog(MT_ChatState, &jack, &jackName, &typingSameDay);
  //  Typing never carries a strip: same-day and next-day stamps match.
  QCOMPARE(d.sizeHint(option, withTyping.index(1, 0)).height(),
           d.sizeHint(option, sameDayTyping.index(1, 0)).height());
}

void TstBubbleHeader::dateStripFallbackMirrorsModel() {
  //  Generic models expose no DateLabelRole: the fallback must mirror
  //  isDateBoundary (timeless-bridge rule + typing guard).
  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QStyleOptionViewItem option;
  option.rect = QRect(0, 0, 400, 10);
  const qint64 d1 = QDateTime(QDate(2024, 1, 10), QTime(12, 0)).toMSecsSinceEpoch();
  const qint64 d2 = QDateTime(QDate(2024, 1, 11), QTime(9, 0)).toMSecsSinceEpoch();
  auto fallbackRow = [](QStandardItemModel& m, int row, qint64 msecs, int type) {
    m.setColumnCount(1);
    const QModelIndex idx = m.index(row, 0);
    m.setData(idx, QStringLiteral("same"), ChatMessageModel::TextRole);
    m.setData(idx, type, ChatMessageModel::TypeRole);
    if(msecs > 0)
      m.setData(idx, QDateTime::fromMSecsSinceEpoch(msecs), ChatMessageModel::TimeRole);
  };

  QStandardItemModel bridged;
  bridged.setRowCount(3);
  fallbackRow(bridged, 0, d1, MT_Message);
  fallbackRow(bridged, 1, 0, MT_Message);
  fallbackRow(bridged, 2, d2, MT_Message);
  QStandardItemModel sameDay;
  sameDay.setRowCount(2);
  fallbackRow(sameDay, 0, d1, MT_Message);
  fallbackRow(sameDay, 1, d1 + 60 * 1000, MT_Message);

  BubbleDelegate d;
  d.setTheme(t);
  //  Direct label checks (independent of theme margins/padding); the
  //  sizeHint compares below additionally pin the dateH integration.
  QVERIFY(!d.dateLabelFor(bridged.index(0, 0)).isEmpty());
  QVERIFY(d.dateLabelFor(sameDay.index(1, 0)).isEmpty());
  QVERIFY(d.dateLabelFor(bridged.index(1, 0)).isEmpty());
  QVERIFY(d.dateLabelFor(bridged.index(2, 0)).isEmpty());
  //  Row 0 opens a strip; same-day continuations and timeless rows do not.
  QVERIFY(d.sizeHint(option, bridged.index(0, 0)).height() >
          d.sizeHint(option, sameDay.index(1, 0)).height());
  //  Timeless-bridge rule: D2 after a timeless predecessor carries no strip,
  //  like a same-day continuation after a dated predecessor.
  QCOMPARE(d.sizeHint(option, bridged.index(2, 0)).height(),
           d.sizeHint(option, sameDay.index(1, 0)).height());
  QCOMPARE(d.sizeHint(option, bridged.index(1, 0)).height(),
           d.sizeHint(option, sameDay.index(1, 0)).height());

  //  Same bridge with an epoch-valid TimeRole (as msgTime emits for missing
  //  XN_TIME): 1970-01-01 is a valid QDate.
  QStandardItemModel epochBridged;
  epochBridged.setRowCount(3);
  epochBridged.setColumnCount(1);
  fallbackRow(epochBridged, 0, d1, MT_Message);
  fallbackRow(epochBridged, 1, 0, MT_Message);
  epochBridged.setData(epochBridged.index(1, 0), QDateTime::fromMSecsSinceEpoch(0),
                       ChatMessageModel::TimeRole);
  fallbackRow(epochBridged, 2, d2, MT_Message);
  QVERIFY(d.dateLabelFor(epochBridged.index(2, 0)).isEmpty());
  QCOMPARE(d.sizeHint(option, epochBridged.index(2, 0)).height(),
           d.sizeHint(option, sameDay.index(1, 0)).height());

  //  Timeless row 0 carries no strip either (cur <= 0).
  QStandardItemModel timelessRow0;
  timelessRow0.setRowCount(1);
  fallbackRow(timelessRow0, 0, 0, MT_Message);
  QVERIFY(d.dateLabelFor(timelessRow0.index(0, 0)).isEmpty());
  QCOMPARE(d.sizeHint(option, timelessRow0.index(0, 0)).height(),
           d.sizeHint(option, bridged.index(1, 0)).height());

  QStandardItemModel nextDayTyping;
  nextDayTyping.setRowCount(2);
  fallbackRow(nextDayTyping, 0, d1, MT_Message);
  fallbackRow(nextDayTyping, 1, d1 + 24 * 60 * 60 * 1000, MT_ChatState);
  QStandardItemModel sameDayTyping;
  sameDayTyping.setRowCount(2);
  fallbackRow(sameDayTyping, 0, d1, MT_Message);
  fallbackRow(sameDayTyping, 1, d1 + 60 * 1000, MT_ChatState);
  //  Typing never carries a strip: next-day and same-day stamps match.
  QVERIFY(d.dateLabelFor(nextDayTyping.index(1, 0)).isEmpty());
  QVERIFY(d.dateLabelFor(sameDayTyping.index(1, 0)).isEmpty());
  QCOMPARE(d.sizeHint(option, nextDayTyping.index(1, 0)).height(),
           d.sizeHint(option, sameDayTyping.index(1, 0)).height());
}

QTEST_MAIN(TstBubbleHeader)
#include "tst_bubbleheader.moc"
