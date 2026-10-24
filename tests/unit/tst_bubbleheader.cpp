// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Header typography in BubbleDelegate: theme + local display size only,
// offscreen. The getters take no model index, so the sender-chosen message
// font cannot leak into name/time by construction.

#include <QtTest>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStyleOptionViewItem>

#include "core/definitions.h"
#include "core/theme.h"
#include "gui/chat/bubbledelegate.h"
#include "gui/chat/chatmessagemodel.h"
#include "messaging/xmlmessage.h"

namespace {

QByteArray meshBlueBytes() {
  QFile f(QStringLiteral(":/themes/MeshBlue/theme.json"));
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
  const char* names[] = {"ClassicSlate", "MeshBlue", "NordicTerminal"};
  for(const char* n : names) {
    QFile f(QString::fromLatin1(":/themes/%1/theme.json").arg(QString::fromLatin1(n)));
    QVERIFY(f.open(QIODevice::ReadOnly));
    Theme t = Theme::loadFromJson(f.readAll());
    QVERIFY2(t.isValid, qPrintable(t.errorString));
    QVERIFY(t.resolve(t.header.senderColor, Theme::Mode::Light, false).isValid());
    QVERIFY(t.resolve(t.header.senderColor, Theme::Mode::Dark, false).isValid());
    QVERIFY(t.resolve(t.header.timeColor, Theme::Mode::Light, false).isValid());
    QVERIFY(t.resolve(t.header.timeColor, Theme::Mode::Dark, false).isValid());
  }
}

void TstBubbleHeader::seriesTopMargins() {
  //  Series of 3 + 1 other: inside the series the dense margin, outside the
  //  series-start margin. MeshBlue ships margin 9 / first 9 / next 3.
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
  QCOMPARE(t.bubble.marginFirstInGroup, 9);
  QCOMPARE(t.bubble.marginNextInGroup, 3);
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

QTEST_MAIN(TstBubbleHeader)
#include "tst_bubbleheader.moc"
