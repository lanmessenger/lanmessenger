// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Text selection in chat bubbles: mouse drag, keyboard, context menu.

#include <QtTest>

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

#include "core/definitions.h"
#include "gui/chat/bubbledelegate.h"
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

void appendText(ChatMessageView& view, const QString& body) {
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  XmlMessage m = textMessage(body);
  view.appendMessageLog(MT_Message, &jack, &jackName, &m);
}

void showView(ChatMessageView& view) {
  view.localId = QStringLiteral("me");
  view.peerId = QStringLiteral("jack");
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));
}

void setupTwoRows(ChatMessageView& view, const QString& first, const QString& second) {
  showView(view);
  appendText(view, first);
  appendText(view, second);
  QCOMPARE(view.rowCount(), 2);
}

XmlMessage fileMessage(const QString& fileId) {
  XmlMessage m;
  m.addHeader(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
  m.addData(XN_FILEID, fileId);
  m.addData(XN_MODE, FileModeNames[FM_Receive]);
  m.addData(XN_FILEOP, FileOpNames[FO_Request]);
  m.addData(XN_FILENAME, QStringLiteral("test_file.txt"));
  m.addData(XN_FILESIZE, QStringLiteral("800000"));
  m.addData(XN_FILEPATH, QStringLiteral("/tmp/test_file.txt"));
  return m;
}

void appendFile(ChatMessageView& view, const QString& fileId) {
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  XmlMessage m = fileMessage(fileId);
  view.appendMessageLog(MT_File, &jack, &jackName, &m);
}

XmlMessage chatStateMessage(ChatState state) {
  XmlMessage m;
  m.addHeader(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
  m.addData(XN_CHATSTATE, ChatStateNames[state]);
  return m;
}

void appendChatState(ChatMessageView& view, ChatState state) {
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  XmlMessage m = chatStateMessage(state);
  view.appendMessageLog(MT_ChatState, &jack, &jackName, &m);
}

QPoint rowCenter(ChatMessageView& view, int row) {
  return view.visualRect(view.model()->index(row, 0)).center();
}

//  QTest::mouseMove(widget, pos, delay) takes a delay, not buttons: on
//  Qt < 6.3 widget moves go through QCursor::setPos, which delivers nothing
//  on offscreen, so a QTest-driven drag never extends there. Send the move
//  directly instead (same viewport -> view path as QTest::mousePress,
//  explicit LeftButton state): works on Qt 6.2 offscreen, 6.4+ and xcb.
void dragMove(ChatMessageView& view, const QPoint& pos) {
  QMouseEvent move(QEvent::MouseMove, pos, view.viewport()->mapToGlobal(pos), Qt::NoButton,
                   Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(view.viewport(), &move);
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
  void ctrlCCopiesSelection();
  void ctrlASelectsCurrentBubble();
  void leftDragSelectsFragment();
  void appendKeepsEarlierSelection();
  void selectedTextNormalizesSpacesAndBreaks();
  void menuCloseResetsContextRow();
  void ctrlAWithInvalidCurrentSelectsLastRow();
  void emptyModelShortcutsAreNoOp();
  void copyWithoutSelectionKeepsClipboard();
  void selectAllEmptyBubbleCopiesNothing();
  void ctrlAAfterCurrentChangeKeepsHighlightRow();
  void tabFocusClearsSelection();
  void tabWithoutAnchorsKeepsSelection();
  void dragBackwardSelectsFragment();
  void dragClampTakesSuffix();
  void dragStaysInStartRow();
  void leftClickMissClearsSelection();
  void rightClickMissKeepsSelection();
  void keyboardMenuSelectAllTargetsCursorRow();
  void linkClickClearsSelectionAndAccepts();
  void typingRowRemovalClearsSelection();
  void fileUpdateMovesCurrentToCursorRow();
  void normalizeHelperUnifiesSpacesAndBreaks();
  void peerRenameKeepsSelection();
};

//  KeyProbe exposes event() return value (swallowed vs propagated).
class KeyProbe : public ChatMessageView {
public:
  using ChatMessageView::ChatMessageView;
  bool sendKey(QKeyEvent* event) {
    return ChatMessageView::event(event);
  }
};

void TstChatSelectAll::rightClickForeignBubbleClearsSelection() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));

  QTest::mousePress(view.viewport(), Qt::RightButton, Qt::NoModifier, rowCenter(view, 1));
  QCOMPARE(view.currentIndex().row(), 1);
  QVERIFY(copiedText(view).isEmpty());
}

void TstChatSelectAll::rightClickOwnBubbleKeepsSelection() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QTest::mousePress(view.viewport(), Qt::RightButton, Qt::NoModifier, rowCenter(view, 0));
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));
}

void TstChatSelectAll::selectAllFromMenuTargetsCursorRow() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

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

void TstChatSelectAll::ctrlCCopiesSelection() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QApplication::clipboard()->clear();
  QTest::keyClick(&view, Qt::Key_C, Qt::ControlModifier);
  QCOMPARE(QApplication::clipboard()->text(),
           QStringLiteral("alpha bravo charlie delta echo"));
}

void TstChatSelectAll::ctrlASelectsCurrentBubble() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(1, 0));
  QTest::keyClick(&view, Qt::Key_A, Qt::ControlModifier);
  QCOMPARE(copiedText(view), QStringLiteral("foxtrot golf hotel india juliet"));
}

void TstChatSelectAll::leftDragSelectsFragment() {
  ChatMessageView view;
  const QString body = QStringLiteral("alpha bravo charlie delta echo");
  setupTwoRows(view, body, QStringLiteral("foxtrot golf hotel india juliet"));

  const QPoint start = rowCenter(view, 0);
  const QPoint end = start + QPoint(80, 0);
  QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, start);
  dragMove(view, end);
  QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, end);
  const QString frag = copiedText(view);
  QVERIFY(!frag.isEmpty());
  QVERIFY(body.contains(frag));
  QVERIFY(frag.size() < body.size());
}

void TstChatSelectAll::appendKeepsEarlierSelection() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  appendText(view, QStringLiteral("kilo lima mike november oscar"));
  QCOMPARE(view.rowCount(), 3);
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));
}

void TstChatSelectAll::selectedTextNormalizesSpacesAndBreaks() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("foo") + QChar(0x00A0) + QStringLiteral("bar\nbaz"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QCOMPARE(copiedText(view), QStringLiteral("foo bar\nbaz"));
}

void TstChatSelectAll::menuCloseResetsContextRow() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  QTimer::singleShot(100, [] {
    for(QWidget* w : QApplication::topLevelWidgets())
      if(auto* menu = qobject_cast<QMenu*>(w))
        menu->close();
  });
  const QPoint pos = rowCenter(view, 1);
  QTest::mousePress(view.viewport(), Qt::RightButton, Qt::NoModifier, pos);
  QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, pos);
  QApplication::sendEvent(view.viewport(), &menuEvent);

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  invoke(view, "selectAllAction_triggered");
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));
}

void TstChatSelectAll::ctrlAWithInvalidCurrentSelectsLastRow() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));
  QVERIFY(!view.currentIndex().isValid());

  QTest::keyClick(&view, Qt::Key_A, Qt::ControlModifier);
  QCOMPARE(copiedText(view), QStringLiteral("foxtrot golf hotel india juliet"));
}

void TstChatSelectAll::emptyModelShortcutsAreNoOp() {
  KeyProbe view;
  showView(view);
  QCOMPARE(view.rowCount(), 0);

  QApplication::clipboard()->setText(QStringLiteral("SENTINEL"));
  QKeyEvent selectAllPress(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
  QVERIFY(view.sendKey(&selectAllPress));
  QKeyEvent copyPress(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
  QVERIFY(view.sendKey(&copyPress));
  QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("SENTINEL"));
}

void TstChatSelectAll::copyWithoutSelectionKeepsClipboard() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  QApplication::clipboard()->setText(QStringLiteral("SENTINEL"));
  QTest::keyClick(&view, Qt::Key_C, Qt::ControlModifier);
  QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("SENTINEL"));
}

void TstChatSelectAll::selectAllEmptyBubbleCopiesNothing() {
  ChatMessageView view;
  showView(view);
  appendText(view, QString());
  QCOMPARE(view.rowCount(), 1);

  view.setCurrentIndex(view.model()->index(0, 0));
  QApplication::clipboard()->setText(QStringLiteral("SENTINEL"));
  invoke(view, "selectAllAction_triggered");
  invoke(view, "copyAction_triggered");
  QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("SENTINEL"));
}

void TstChatSelectAll::ctrlAAfterCurrentChangeKeepsHighlightRow() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  //  Arrow keys move current, not the highlight: Ctrl+A acts on the
  //  visible selection (row 0), not the new current row.
  view.setCurrentIndex(view.model()->index(1, 0));
  invoke(view, "selectAllAction_triggered");
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));
}

void TstChatSelectAll::tabFocusClearsSelection() {
  KeyProbe view;
  showView(view);
  appendText(view, QStringLiteral("alpha bravo charlie delta echo"));
  appendFile(view, QStringLiteral("fid-tab"));
  QCOMPARE(view.rowCount(), 2);

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QSignalSpy sent(&view, &ChatMessageView::messageSent);
  QKeyEvent tabPress(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
  QVERIFY(view.sendKey(&tabPress));
  QVERIFY(copiedText(view).isEmpty());
  //  Focus landed on the Accept anchor: Return activates it.
  QKeyEvent returnPress(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
  QVERIFY(view.sendKey(&returnPress));
  QCOMPARE(sent.count(), 1);
}

void TstChatSelectAll::tabWithoutAnchorsKeepsSelection() {
  KeyProbe view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QKeyEvent tabPress(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
  QVERIFY(!view.sendKey(&tabPress));
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));
}

void TstChatSelectAll::dragBackwardSelectsFragment() {
  ChatMessageView view;
  const QString body = QStringLiteral("alpha bravo charlie delta echo");
  setupTwoRows(view, body, QStringLiteral("foxtrot golf hotel india juliet"));

  const QPoint start = rowCenter(view, 0) + QPoint(80, 0);
  const QPoint end = start - QPoint(160, 0);
  QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, start);
  dragMove(view, end);
  QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, end);
  const QString frag = copiedText(view);
  QVERIFY(!frag.isEmpty());
  QVERIFY(body.contains(frag));
  QVERIFY(frag.size() < body.size());
}

void TstChatSelectAll::dragClampTakesSuffix() {
  ChatMessageView view;
  const QString body = QStringLiteral("alpha bravo charlie delta echo");
  setupTwoRows(view, body, QStringLiteral("foxtrot golf hotel india juliet"));

  const QPoint start = rowCenter(view, 0);
  const int below = view.visualRect(view.model()->index(0, 0)).bottom() + 40;
  const QPoint end(start.x(), below);
  QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, start);
  dragMove(view, end);
  QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, end);
  const QString frag = copiedText(view);
  QVERIFY(!frag.isEmpty());
  QVERIFY(body.endsWith(frag));
}

void TstChatSelectAll::dragStaysInStartRow() {
  ChatMessageView view;
  const QString body = QStringLiteral("alpha bravo charlie delta echo");
  setupTwoRows(view, body, QStringLiteral("foxtrot golf hotel india juliet"));

  const QPoint start = rowCenter(view, 0);
  const QPoint end = rowCenter(view, 1);
  QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, start);
  dragMove(view, end);
  QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, end);
  const QString frag = copiedText(view);
  QVERIFY(!frag.isEmpty());
  QVERIFY(body.contains(frag));
}

void TstChatSelectAll::leftClickMissClearsSelection() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  const QPoint miss(200, view.viewport()->height() - 10);
  QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, miss);
  QVERIFY(copiedText(view).isEmpty());
}

void TstChatSelectAll::rightClickMissKeepsSelection() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  const QPoint miss(200, view.viewport()->height() - 10);
  QTest::mouseClick(view.viewport(), Qt::RightButton, Qt::NoModifier, miss);
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));
}

void TstChatSelectAll::keyboardMenuSelectAllTargetsCursorRow() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QTimer::singleShot(100, [] {
    for(QWidget* w : QApplication::topLevelWidgets())
      if(auto* menu = qobject_cast<QMenu*>(w))
        menu->close();
  });
  //  No right-button press: menu opens over row 1, closes without action.
  const QPoint pos = rowCenter(view, 1);
  QContextMenuEvent menuEvent(QContextMenuEvent::Keyboard, pos);
  QApplication::sendEvent(view.viewport(), &menuEvent);
  QCOMPARE(view.currentIndex().row(), 0);
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));

  //  Same menu, now act: Select All targets the cursor row via contextRow.
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
  QContextMenuEvent menuEvent2(QContextMenuEvent::Keyboard, pos);
  QApplication::sendEvent(view.viewport(), &menuEvent2);
  QCOMPARE(copiedText(view), QStringLiteral("foxtrot golf hotel india juliet"));
}

void TstChatSelectAll::linkClickClearsSelectionAndAccepts() {
  ChatMessageView view;
  showView(view);
  appendText(view, QStringLiteral("alpha bravo charlie delta echo"));
  appendFile(view, QStringLiteral("fid-link"));
  QCOMPARE(view.rowCount(), 2);

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QSignalSpy sent(&view, &ChatMessageView::messageSent);
  //  Accept sits left of Decline at the card bottom: sweep until it fires.
  const QRect rowRect = view.visualRect(view.model()->index(1, 0));
  const int lines[2] = {rowRect.bottom() - 12, rowRect.center().y()};
  for(int line : lines) {
    for(int x = rowRect.left() + 5; x < rowRect.right() - 3 && sent.isEmpty(); x += 6)
      QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(x, line));
  }
  QCOMPARE(sent.count(), 1);
  QVERIFY(copiedText(view).isEmpty());
}

void TstChatSelectAll::typingRowRemovalClearsSelection() {
  ChatMessageView view;
  showView(view);
  appendText(view, QStringLiteral("alpha bravo charlie delta echo"));
  appendChatState(view, CS_Composing);
  QCOMPARE(view.rowCount(), 2);

  view.setCurrentIndex(view.model()->index(1, 0));
  invoke(view, "selectAllAction_triggered");
  QVERIFY(!copiedText(view).isEmpty());
  //  Typing stops with no message: the row is removed, not replaced.
  appendChatState(view, CS_Active);
  QCOMPARE(view.rowCount(), 1);

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));
}

void TstChatSelectAll::fileUpdateMovesCurrentToCursorRow() {
  ChatMessageView view;
  showView(view);
  appendText(view, QStringLiteral("alpha bravo charlie delta echo"));
  appendFile(view, QStringLiteral("fid-update"));
  QCOMPARE(view.rowCount(), 2);

  view.setCurrentIndex(view.model()->index(1, 0));
  invoke(view, "selectAllAction_triggered");
  QVERIFY(!copiedText(view).isEmpty());
  QString jack = QStringLiteral("jack");
  view.updateFileMessage(FM_Receive, FO_Accept, QStringLiteral("fid-update"));

  //  Card text changed under the selection: it must be gone, so the
  //  keyboard menu follows the cursor row again.
  view.setCurrentIndex(view.model()->index(0, 0));
  QTimer::singleShot(100, [] {
    for(QWidget* w : QApplication::topLevelWidgets())
      if(auto* menu = qobject_cast<QMenu*>(w))
        menu->close();
  });
  const QPoint pos = rowCenter(view, 1);
  QContextMenuEvent menuEvent(QContextMenuEvent::Keyboard, pos);
  QApplication::sendEvent(view.viewport(), &menuEvent);
  QCOMPARE(view.currentIndex().row(), 1);
}

void TstChatSelectAll::normalizeHelperUnifiesSpacesAndBreaks() {
  const QString raw = QStringLiteral("a") + QChar(0x00A0) + QStringLiteral("b") +
                      QChar(0x2028) + QStringLiteral("c") + QChar(0x2029) +
                      QStringLiteral("d");
  QCOMPARE(BubbleDelegate::normalizePlainText(raw), QStringLiteral("a b\nc\nd"));
}

void TstChatSelectAll::peerRenameKeepsSelection() {
  ChatMessageView view;
  setupTwoRows(view, QStringLiteral("alpha bravo charlie delta echo"),
               QStringLiteral("foxtrot golf hotel india juliet"));

  view.setCurrentIndex(view.model()->index(0, 0));
  invoke(view, "selectAllAction_triggered");
  QString jack = QStringLiteral("jack");
  QString jackRenamed = QStringLiteral("Jack the Second");
  view.updateUserName(&jack, &jackRenamed);
  QCOMPARE(copiedText(view), QStringLiteral("alpha bravo charlie delta echo"));
}

QTEST_MAIN(TstChatSelectAll)
#include "tst_chatselectall.moc"
