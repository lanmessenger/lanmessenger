// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Classic -> scroll to bottom -> Nordic left a stale offset/row geometry
// painting a white box over the sibling group (issue #106).
// relayoutPreview() must pin to the top with a synchronous layout. Offscreen.

#include <QtTest>

#include <QScrollBar>

#include "core/definitions.h"
#include "gui/chat/chatdefinitions.h"
#include "gui/chat/chatmessageview.h"
#include "messaging/xmlmessage.h"

namespace {

XmlMessage textMessage(const QString& text) {
  XmlMessage m;
  m.addHeader(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
  m.addData(XN_MESSAGE, text);
  m.addData(XN_FONT, QFont().toString());
  m.addData(XN_COLOR, QStringLiteral("#000000"));
  return m;
}

//  Mirrors the refreshPreview mock log, enough rows to scroll 400x300.
void appendPreviewRows(ChatMessageView& view) {
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Myself");
  for(int i = 0; i < 6; i++) {
    XmlMessage incoming =
      textMessage(QStringLiteral("Hello, this is incoming message number %1 with some body text.").arg(i));
    view.appendMessageLog(MT_Message, &jack, &jackName, &incoming);
    XmlMessage outgoing =
      textMessage(QStringLiteral("Hi, this is outgoing message number %1 with some body text.").arg(i));
    view.appendMessageLog(MT_Message, &me, &meName, &outgoing);
  }
}

} // namespace

class TstThemePreview : public QObject {
  Q_OBJECT

private slots:
  void themeSwitchFromBottomPinsToTop();
};

void TstThemePreview::themeSwitchFromBottomPinsToTop() {
  ChatMessageView view;
  view.localId = QStringLiteral("me");
  view.peerId = QStringLiteral("jack");
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));

  view.setThemeName(QStringLiteral("ClassicSlate"));
  view.setAppearanceMode(QStringLiteral("system"));
  view.initChat();
  appendPreviewRows(view);
  QCOMPARE(view.rowCount(), 12);
  view.doItemsLayout();
  view.scrollToBottom();
  QVERIFY(view.verticalScrollBar()->maximum() > 0);
  QCOMPARE(view.verticalScrollBar()->value(), view.verticalScrollBar()->maximum());

  //  Theme switch + full rebuild, then the stale bottom offset.
  view.setThemeName(QStringLiteral("NordicTerminal"));
  view.setAppearanceMode(QStringLiteral("system"));
  view.initChat();
  appendPreviewRows(view);
  QCOMPARE(view.rowCount(), 12);

  view.scrollToBottom();
  QCOMPARE(view.verticalScrollBar()->value(), view.verticalScrollBar()->maximum());

  view.relayoutPreview();

  QCOMPARE(view.verticalScrollBar()->value(), view.verticalScrollBar()->minimum());
  QVERIFY(view.verticalScrollBar()->value() <= view.verticalScrollBar()->maximum());
  //  No blank gap at the top.
  QVERIFY(view.indexAt(QPoint(10, 10)).isValid());
  QVERIFY(view.visualRect(view.model()->index(0, 0)).top() >= 0);
}

QTEST_MAIN(TstThemePreview)
#include "tst_themepreview.moc"
