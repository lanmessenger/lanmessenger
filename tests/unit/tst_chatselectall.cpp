// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Right-click + Select All targets the bubble under the cursor.

#include <QtTest>

#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QTimer>

#include "core/definitions.h"
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

QPoint rowCenter(ChatMessageView& view, int row) {
  return view.visualRect(view.model()->index(row, 0)).center();
}

void invoke(ChatMessageView& view, const char* slot) {
  QVERIFY(QMetaObject::invokeMethod(&view, slot));
}

QString copiedText(ChatMessageView& view) {
  QApplication::clipboard()->clear();
  invoke(view, "copyAction_triggered");
  return QApplication::clipboard()->text();
}

} // namespace

class TstChatSelectAll : public QObject {
  Q_OBJECT

private slots:
  void rightClickForeignBubbleClearsSelection();
  void rightClickOwnBubbleKeepsSelection();
  void selectAllFromMenuTargetsCursorRow();
};

void TstChatSelectAll::rightClickForeignBubbleClearsSelection() {
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
  XmlMessage m1 = textMessage(QStringLiteral("alpha bravo charlie delta echo"));
  XmlMessage m2 = textMessage(QStringLiteral("foxtrot golf hotel india juliet"));
  view.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  view.appendMessageLog(MT_Message, &jack, &jackName, &m2);
  QCOMPARE(view.rowCount(), 2);

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));

  QTest::mousePress(view.viewport(), Qt::RightButton, Qt::NoModifier, rowCenter(view, 1));
  QCOMPARE(view.currentIndex().row(), 1);
  QVERIFY(copiedText(view).isEmpty());
}

void TstChatSelectAll::rightClickOwnBubbleKeepsSelection() {
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
  XmlMessage m1 = textMessage(QStringLiteral("alpha bravo charlie delta echo"));
  XmlMessage m2 = textMessage(QStringLiteral("foxtrot golf hotel india juliet"));
  view.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  view.appendMessageLog(MT_Message, &jack, &jackName, &m2);

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QTest::mousePress(view.viewport(), Qt::RightButton, Qt::NoModifier, rowCenter(view, 0));
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));
}

void TstChatSelectAll::selectAllFromMenuTargetsCursorRow() {
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
  XmlMessage m1 = textMessage(QStringLiteral("alpha bravo charlie delta echo"));
  XmlMessage m2 = textMessage(QStringLiteral("foxtrot golf hotel india juliet"));
  view.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  view.appendMessageLog(MT_Message, &jack, &jackName, &m2);

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));

  QTimer::singleShot(100, [] {
    for(QWidget* w : QApplication::topLevelWidgets()) {
      auto* menu = qobject_cast<QMenu*>(w);
      if(menu == nullptr)
        continue;
      for(QAction* a : menu->actions()) {
        if(a->shortcut() == QKeySequence::SelectAll) {
          a->trigger();
          menu->close();
          return;
        }
      }
      menu->close();
    }
  });
  const QPoint pos = rowCenter(view, 1);
  QTest::mousePress(view.viewport(), Qt::RightButton, Qt::NoModifier, pos);
  QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, pos);
  QApplication::sendEvent(view.viewport(), &menuEvent);
  QCOMPARE(copiedText(view), QStringLiteral("foxtrot golf hotel india juliet"));
}

QTEST_MAIN(TstChatSelectAll)
#include "tst_chatselectall.moc"
