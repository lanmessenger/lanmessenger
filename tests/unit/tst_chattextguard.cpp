// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Receive-side guard for peer-chosen XN_FONT/XN_COLOR: size clamp, opaque
// colors, WCAG contrast floor. Offscreen, no network.

#include <QtTest>

#include <QFile>

#include "core/definitions.h"
#include "gui/chat/bubbledelegate.h"
#include "gui/chat/chatmessagemodel.h"
#include "gui/chat/chattextguard.h"
#include "messaging/xmlmessage.h"

namespace {

XmlMessage styledMessage(const QString& fontStr, const QString& colorStr) {
  XmlMessage m;
  m.addHeader(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
  m.addData(XN_MESSAGE, QStringLiteral("hi"));
  if(!fontStr.isNull())
    m.addData(XN_FONT, fontStr);
  if(!colorStr.isNull())
    m.addData(XN_COLOR, colorStr);
  return m;
}

QString fontString(double pt) {
  QFont f;
  f.setPointSizeF(pt);
  return f.toString();
}

QByteArray meshBlueBytes() {
  QFile f(QStringLiteral(":/themes/MeshBlue.json"));
  if(!f.open(QIODevice::ReadOnly))
    return QByteArray();
  return f.readAll();
}

QString renderedColor(ChatMessageModel& model, int row, Theme::Mode mode) {
  Theme t = Theme::loadFromJson(meshBlueBytes());
  BubbleDelegate d;
  d.setTheme(t);
  d.setMode(mode, false);
  QModelIndex idx = model.index(row, 0);
  return d.rowHtml(idx, d.resolveRow(idx));
}

} // namespace

class TstChatTextGuard : public QObject {
  Q_OBJECT

private slots:
  void contrastBlackWhite();
  void unreadablePairs();
  void readablePairs();
  void fontClamp();
  void fontUnknownFamilyHarmless();
  void colorOpaque();
  void modelWiring();
  void rowHtmlOutgoingFallback();
  void rowHtmlIncomingDarkFallback();
  void rowHtmlIncomingRedStays();
  void rowHtmlTransparentSystemStays();
  void rowHtmlExplicitTransparentFallback();
};

void TstChatTextGuard::contrastBlackWhite() {
  double r = ChatTextGuard::contrastRatio(QColor(QStringLiteral("#000000")),
                                          QColor(QStringLiteral("#FFFFFF")));
  QVERIFY(r > 20.9 && r < 21.1);
  QCOMPARE(ChatTextGuard::contrastRatio(QColor(QStringLiteral("#FFFFFF")),
                                        QColor(QStringLiteral("#FFFFFF"))),
           1.0);
}

void TstChatTextGuard::unreadablePairs() {
  QVERIFY(!ChatTextGuard::textReadableOn(QColor(QStringLiteral("#000000")),
                                         QColor(QStringLiteral("#1F242B"))));
  QVERIFY(!ChatTextGuard::textReadableOn(QColor(QStringLiteral("#FFFFFF")),
                                         QColor(QStringLiteral("#FFFFFF"))));
  QVERIFY(!ChatTextGuard::textReadableOn(QColor(QStringLiteral("#FFFF00")),
                                         QColor(QStringLiteral("#D9F2C7"))));
}

void TstChatTextGuard::readablePairs() {
  QVERIFY(ChatTextGuard::textReadableOn(QColor(QStringLiteral("#1A1D21")),
                                        QColor(QStringLiteral("#FFFFFF"))));
  QVERIFY(ChatTextGuard::textReadableOn(QColor(QStringLiteral("#FFFFFF")),
                                        QColor(QStringLiteral("#1F242B"))));
  QVERIFY(ChatTextGuard::textReadableOn(QColor(QStringLiteral("#FF0000")),
                                        QColor(QStringLiteral("#FFFFFF"))));
}

void TstChatTextGuard::fontClamp() {
  QFont huge;
  huge.setPointSizeF(72.0);
  QCOMPARE(ChatTextGuard::sanitizeIncomingFont(huge).pointSizeF(), 24.0);

  QFont tiny;
  tiny.setPointSizeF(1.0);
  QCOMPARE(ChatTextGuard::sanitizeIncomingFont(tiny).pointSizeF(), 8.0);

  QFont normal(QStringLiteral("DejaVu Sans"));
  normal.setPointSizeF(10.0);
  normal.setBold(true);
  QFont out = ChatTextGuard::sanitizeIncomingFont(normal);
  QCOMPARE(out.pointSizeF(), 10.0);
  QCOMPARE(out.family(), QStringLiteral("DejaVu Sans"));
  QVERIFY(out.bold());

  QFont px;
  px.setPixelSize(96);
  QCOMPARE(ChatTextGuard::sanitizeIncomingFont(px).pointSizeF(), 24.0);
}

void TstChatTextGuard::fontUnknownFamilyHarmless() {
  QFont junk;
  QVERIFY(junk.fromString(QStringLiteral("NoSuchFamilyXYZ,12")));
  QFont out = ChatTextGuard::sanitizeIncomingFont(junk);
  QCOMPARE(out.family(), QStringLiteral("NoSuchFamilyXYZ"));
  QCOMPARE(out.pointSizeF(), 12.0);
}

void TstChatTextGuard::colorOpaque() {
  QColor invalid(QStringLiteral("not-a-color"));
  QVERIFY(!ChatTextGuard::sanitizeIncomingColor(invalid).isValid());

  QColor ghost(QStringLiteral("#00FF0000"));
  QVERIFY(ghost.isValid());
  QColor solid = ChatTextGuard::sanitizeIncomingColor(ghost);
  QVERIFY(solid.isValid());
  QCOMPARE(solid.alpha(), 255);
  QCOMPARE(solid.red(), 255);
  QCOMPARE(solid.green(), 0);
  QCOMPARE(solid.blue(), 0);

  QColor red(QStringLiteral("#FF0000"));
  QCOMPARE(ChatTextGuard::sanitizeIncomingColor(red).alpha(), 255);
  QCOMPARE(ChatTextGuard::sanitizeIncomingColor(red).name().toLower(),
           QStringLiteral("#ff0000"));
}

void TstChatTextGuard::modelWiring() {
  ChatMessageModel model;
  QString peer = QStringLiteral("peer");
  QString peerName = QStringLiteral("Peer");

  //  Absent styling: theme fallback on both roles.
  XmlMessage plain = styledMessage(QString(), QString());
  model.appendMessageLog(MT_Message, &peer, &peerName, &plain);
  QVERIFY(!model.data(model.index(0, 0), ChatMessageModel::FontRole).isValid());
  QVERIFY(
    !model.data(model.index(0, 0), ChatMessageModel::ColorRole).value<QColor>().isValid());

  //  Absurd size clamps through the model; color passes through opaque.
  XmlMessage poisoned = styledMessage(fontString(72.0), QStringLiteral("#FF0000"));
  model.appendMessageLog(MT_Message, &peer, &peerName, &poisoned);
  QFont f =
    model.data(model.index(1, 0), ChatMessageModel::FontRole).value<QFont>();
  QCOMPARE(f.pointSizeF(), 24.0);
  QColor c =
    model.data(model.index(1, 0), ChatMessageModel::ColorRole).value<QColor>();
  QVERIFY(c.isValid());

  //  Tiny size clamps up; transparent color is forced opaque.
  XmlMessage sneaky = styledMessage(fontString(4.0), QStringLiteral("#00FF0000"));
  model.appendMessageLog(MT_Message, &peer, &peerName, &sneaky);
  QFont f2 =
    model.data(model.index(2, 0), ChatMessageModel::FontRole).value<QFont>();
  QCOMPARE(f2.pointSizeF(), 8.0);
  QColor c2 =
    model.data(model.index(2, 0), ChatMessageModel::ColorRole).value<QColor>();
  QVERIFY(c2.isValid());
  QCOMPARE(c2.alpha(), 255);

  //  Unparsable XN_FONT: theme fallback.
  XmlMessage broken = styledMessage(QStringLiteral(",,,,,"), QString());
  model.appendMessageLog(MT_Message, &peer, &peerName, &broken);
  QVERIFY(!model.data(model.index(3, 0), ChatMessageModel::FontRole).isValid());
}

void TstChatTextGuard::rowHtmlOutgoingFallback() {
  ChatMessageModel model;
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  model.localId = me;
  XmlMessage m = styledMessage(QString(), QStringLiteral("#0369A1"));
  model.appendMessageLog(MT_Message, &me, &meName, &m);
  QString html = renderedColor(model, 0, Theme::Mode::Light);
  QVERIFY(html.contains(QStringLiteral("color:#ffffff")));
  QVERIFY(!html.contains(QStringLiteral("#0369a1")));
}

void TstChatTextGuard::rowHtmlIncomingDarkFallback() {
  ChatMessageModel model;
  QString peer = QStringLiteral("peer");
  QString peerName = QStringLiteral("Peer");
  model.localId = QStringLiteral("me");
  XmlMessage m = styledMessage(QString(), QStringLiteral("#000000"));
  model.appendMessageLog(MT_Message, &peer, &peerName, &m);
  QString html = renderedColor(model, 0, Theme::Mode::Dark);
  QVERIFY(html.contains(QStringLiteral("color:#e2e8f0")));
  QVERIFY(!html.contains(QStringLiteral("#000000")));
}

void TstChatTextGuard::rowHtmlIncomingRedStays() {
  ChatMessageModel model;
  QString peer = QStringLiteral("peer");
  QString peerName = QStringLiteral("Peer");
  model.localId = QStringLiteral("me");
  XmlMessage m = styledMessage(QString(), QStringLiteral("#FF0000"));
  model.appendMessageLog(MT_Message, &peer, &peerName, &m);
  QString html = renderedColor(model, 0, Theme::Mode::Light);
  QVERIFY(html.contains(QStringLiteral("color:#ff0000")));
  QVERIFY(!html.contains(QStringLiteral("color:#0f172a")));
}

void TstChatTextGuard::rowHtmlTransparentSystemStays() {
  ChatMessageModel model;
  QString peer = QStringLiteral("peer");
  QString peerName = QStringLiteral("Peer");
  model.localId = QStringLiteral("me");
  XmlMessage m = styledMessage(QString(), QStringLiteral("#FFFF00"));
  model.appendMessageLog(MT_Failed, &peer, &peerName, &m);
  QString html = renderedColor(model, 0, Theme::Mode::Light);
  QVERIFY(html.contains(QStringLiteral("color:#64748b")));
  QVERIFY(!html.contains(QStringLiteral("#ffff00")));
}

void TstChatTextGuard::rowHtmlExplicitTransparentFallback() {
  ChatMessageModel model;
  QString peer = QStringLiteral("peer");
  QString peerName = QStringLiteral("Peer");
  model.localId = QStringLiteral("me");
  XmlMessage m = styledMessage(QString(), QStringLiteral("#FFFF00"));
  model.appendMessageLog(MT_Message, &peer, &peerName, &m);
  Theme t = Theme::loadFromJson(meshBlueBytes());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  BubbleDelegate d;
  d.setTheme(t);
  d.setMode(Theme::Mode::Light, false);
  BubbleDelegate::ResolvedRow row;
  row.card = QStringLiteral("incoming");
  row.text = Theme::DualColor{QColor(QStringLiteral("#0F172A")),
                              QColor(QStringLiteral("#E2E8F0"))};
  row.bg.kind = Theme::Bg::Kind::Transparent;
  QModelIndex idx = model.index(0, 0);
  QString html = d.rowHtml(idx, row);
  QVERIFY(html.contains(QStringLiteral("color:#0f172a")));
  QVERIFY(!html.contains(QStringLiteral("#ffff00")));
}

QTEST_MAIN(TstChatTextGuard)
#include "tst_chattextguard.moc"
