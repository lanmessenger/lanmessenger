// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Keyboard anchor navigation in ChatMessageView (Tab/Shift+Tab/Return),
// offscreen: no display needed.

#include <QtTest>

#include "core/definitions.h"
#include "gui/chat/chatmessageview.h"
#include "messaging/xmlmessage.h"

namespace {

XmlMessage fileMessage(const QString& fileId, const QString& mode, const QString& op) {
  XmlMessage m;
  m.addHeader(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
  m.addData(XN_FILEID, fileId);
  m.addData(XN_MODE, mode);
  m.addData(XN_FILEOP, op);
  m.addData(XN_FILENAME, QStringLiteral("test_file.txt"));
  m.addData(XN_FILESIZE, QStringLiteral("800000"));
  m.addData(XN_FILEPATH, QStringLiteral("/tmp/test_file.txt"));
  return m;
}

} // namespace

class NavProbe : public ChatMessageView {
public:
  using ChatMessageView::ChatMessageView;
  bool sendKey(QKeyEvent* event) {
    return ChatMessageView::event(event);
  }
};

class TstChatNav : public QObject {
  Q_OBJECT

private slots:
  void tabReturnAccepts();
  void tabPastLastAnchorLeavesView();
  void viewportBackgroundMatchesPage();
  void legacyHtmlShowsAsSingleRow();
};

void TstChatNav::tabReturnAccepts() {
  ChatMessageView view;
  view.localId = QStringLiteral("me");
  view.peerId = QStringLiteral("jack");
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));

  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");

  //  Send row (Cancel) + receive row (Accept/Decline), as in the loopback flow.
  XmlMessage send = fileMessage(QStringLiteral("fid1"), FileModeNames[FM_Send],
                                FileOpNames[FO_Request]);
  XmlMessage recv = fileMessage(QStringLiteral("fid2"), FileModeNames[FM_Receive],
                                FileOpNames[FO_Request]);
  view.appendMessageLog(MT_File, &me, &meName, &send);
  view.appendMessageLog(MT_File, &jack, &jackName, &recv);
  QCOMPARE(view.rowCount(), 2);

  QSignalSpy sent(&view, &ChatMessageView::messageSent);
  //  Tab #1 -> Cancel (send row), Tab #2 -> Accept (receive row).
  QTest::keyClick(&view, Qt::Key_Tab);
  QTest::keyClick(&view, Qt::Key_Tab);
  QTest::keyClick(&view, Qt::Key_Return);
  QTRY_COMPARE_WITH_TIMEOUT(sent.count(), 1, 5000);

  //  Accept sent: the receive row shows Accepted.
  QVERIFY(view.plainText().contains(QStringLiteral("Accepted")));
}

void TstChatNav::tabPastLastAnchorLeavesView() {
  NavProbe view;
  view.localId = QStringLiteral("me");
  view.peerId = QStringLiteral("jack");
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));

  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  XmlMessage send = fileMessage(QStringLiteral("fid1"), FileModeNames[FM_Send],
                                FileOpNames[FO_Request]);
  XmlMessage recv = fileMessage(QStringLiteral("fid2"), FileModeNames[FM_Receive],
                                FileOpNames[FO_Request]);
  view.appendMessageLog(MT_File, &me, &meName, &send);
  view.appendMessageLog(MT_File, &jack, &jackName, &recv);

  //  Anchors in order: Cancel, Accept, Decline. Past the last one the key
  //  event must stay unhandled so Qt moves focus out of the view (no trap).
  QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
  QVERIFY(view.sendKey(&tab));
  QVERIFY(view.sendKey(&tab));
  QVERIFY(view.sendKey(&tab));
  QVERIFY(!view.sendKey(&tab));

  //  Shift+Tab from the cleared state re-enters at the last anchor.
  QKeyEvent backtab(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier);
  QVERIFY(view.sendKey(&backtab));
}

void TstChatNav::viewportBackgroundMatchesPage() {
  ChatMessageView view;
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));
  view.initChat();

  view.setAppearanceMode(QStringLiteral("light"));
  QCOMPARE(view.viewport()->palette().color(QPalette::Base),
           QColor(QStringLiteral("#EDEFF2")));

  view.setAppearanceMode(QStringLiteral("dark"));
  QCOMPARE(view.viewport()->palette().color(QPalette::Base),
           QColor(QStringLiteral("#131518")));
}

void TstChatNav::legacyHtmlShowsAsSingleRow() {
  ChatMessageView view;
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));
  view.initChat();

  //  S1.5 history: stored blobs (legacy or new-model HTML) render as one
  //  read-only body row, never migrated.
  QString name = QStringLiteral("Jack");
  QDateTime when = QDateTime::currentDateTime();
  QString html = QStringLiteral("<html><body><p><span class='salutation'>Jack:</span>"
                                "<span class='time'>12:00</span>"
                                "<span class='message'>hi there</span></p></body></html>");
  view.showLegacyHtml(&name, &when, &html);

  QCOMPARE(view.rowCount(), 1);
  QVERIFY(view.messageAt(0).contains(QStringLiteral("hi there")));
  //  Stored blobs carry no own-flag: history never reads as outgoing.
  QVERIFY(!view.isOwnRow(0));

  //  Broken offset / wiped historyFile: no phantom empty bubble.
  QString empty;
  view.showLegacyHtml(&name, &when, &empty);
  QCOMPARE(view.rowCount(), 0);
}

QTEST_MAIN(TstChatNav)
#include "tst_chatnav.moc"
