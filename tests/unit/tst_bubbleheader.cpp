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
#include <QStyleOptionViewItem>

#include "core/definitions.h"
#include "core/theme.h"
#include "gui/chat/bubbledelegate.h"
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
  void paintRowsOffscreen();
};

XmlMessage textMessage(const QString& text) {
  XmlMessage m;
  m.addHeader(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
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
  QVERIFY(name.bold());
  QVERIFY(!time.bold());
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
  const char* names[] = {"Bubble", "ClassicSlate", "MeshBlue", "NordicTerminal"};
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
    QVERIFY(Theme::pick(t.link, true).isValid());
    QVERIFY(Theme::pick(t.page, false).isValid());
  }
}

void TstBubbleHeader::seriesTopMargins() {
  //  Series of 3 + 1 other: inside the series the dense margin, outside the
  //  series-start margin. MeshBlue ships margin 9 on series-start trees,
  //  3 on the _next trees.
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
  QCOMPARE(d.rowTopMargin(model.index(0, 0)), 9);
  QCOMPARE(d.rowTopMargin(model.index(1, 0)), 3);
  QCOMPARE(d.rowTopMargin(model.index(2, 0)), 3);
  QCOMPARE(d.rowTopMargin(model.index(3, 0)), 9);

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
  int first[4] = {0, 0, 0, 0};
  int next[4] = {0, 0, 0, 0};
  int single[4] = {0, 0, 0, 0};
  int ownNext[4] = {0, 0, 0, 0};
  d.rowCornerRadii(model.index(0, 0), first);
  d.rowCornerRadii(model.index(1, 0), next);
  d.rowCornerRadii(model.index(3, 0), single);
  d.rowCornerRadii(model.index(4, 0), ownNext);
  const int wantBase[4] = {14, 14, 14, 3};
  const int wantNext[4] = {3, 14, 14, 3};
  //  Own rows mirror the base too: tail side is right.
  const int wantOwnSingle[4] = {14, 14, 3, 14};
  const int wantOwnNext[4] = {14, 3, 3, 14};
  for(int i = 0; i < 4; i++) {
    QCOMPARE(first[i], wantBase[i]);
    QCOMPARE(next[i], wantNext[i]);
    QCOMPARE(single[i], wantOwnSingle[i]);
    QCOMPARE(ownNext[i], wantOwnNext[i]);
  }
  //  Third row is grouped too: same radii as the second.
  int third[4] = {0, 0, 0, 0};
  d.rowCornerRadii(model.index(2, 0), third);
  for(int i = 0; i < 4; i++)
    QCOMPARE(third[i], wantNext[i]);
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
  int single[4] = {0, 0, 0, 0};
  int next[4] = {0, 0, 0, 0};
  d.rowCornerRadii(model.index(1, 0), single);
  d.rowCornerRadii(model.index(2, 0), next);
  const int wantSingle[4] = {1, 2, 3, 4};
  const int wantNext[4] = {5, 6, 7, 8};
  for(int i = 0; i < 4; i++) {
    QCOMPARE(single[i], wantSingle[i]);
    QCOMPARE(next[i], wantNext[i]);
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
  QCOMPARE(first.shadowDy, 2.0);
  QCOMPARE(first.shadowBlur, 4.0);
  QCOMPARE(first.shadowOpacity, 0.08);
  QCOMPARE(first.margin, 9);
  QCOMPARE(first.maxWidthPct, 80);
  QVERIFY(!first.hasBorder);

  BubbleDelegate::ResolvedRow next = d.resolveRow(model.index(1, 0));
  //  In-series rows walk layout_incoming_next: no avatar, no header.
  QVERIFY(!next.avatar);
  QVERIFY(!next.header);
  QVERIFY(next.headerItems.isEmpty());
  QCOMPARE(next.margin, 3);

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

QTEST_MAIN(TstBubbleHeader)
#include "tst_bubbleheader.moc"
