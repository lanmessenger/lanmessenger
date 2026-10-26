// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Unit tests for ChatMessageModel (grouping, plainText, files).

#include <QtTest>

#include "core/definitions.h"
#include "gui/chat/chatdefinitions.h"
#include "gui/chat/chatmessagemodel.h"
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

XmlMessage fileRequest(const QString& fileId, const QString& name) {
  XmlMessage m;
  m.addHeader(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
  m.addData(XN_FILEID, fileId);
  m.addData(XN_MODE, FileModeNames[FM_Receive]);
  m.addData(XN_FILEOP, FileOpNames[FO_Request]);
  m.addData(XN_FILENAME, name);
  m.addData(XN_FILESIZE, QStringLiteral("800000"));
  m.addData(XN_FILEPATH, QStringLiteral("/tmp/") + name);
  return m;
}

XmlMessage chatState(const QString& state) {
  XmlMessage m;
  m.addData(XN_CHATSTATE, state);
  return m;
}

} // namespace

class TstChatModel : public QObject {
  Q_OBJECT

private slots:
  void grouping();
  void plainTextJoins();
  void fileFlow();
  void folderKeepsType();
  void typingRowShowsAndClears();
  void typingPausedText();
  void typingActiveClears();
  void typingNotStored();
  void typingDoesNotGroup();
  void textSigStable();
};


void TstChatModel::grouping() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  model.localId = me;

  XmlMessage m1 = textMessage(QStringLiteral("hi"));
  QCOMPARE(model.appendMessageLog(MT_Message, &jack, &jackName, &m1), 0);
  XmlMessage m2 = textMessage(QStringLiteral("again"));
  QCOMPARE(model.appendMessageLog(MT_Message, &jack, &jackName, &m2), 1);
  XmlMessage m3 = textMessage(QStringLiteral("hello"));
  QCOMPARE(model.appendMessageLog(MT_Message, &me, &meName, &m3), 2);

  QCOMPARE(model.data(model.index(0, 0), ChatMessageModel::GroupedRole).toBool(), false);
  QCOMPARE(model.data(model.index(1, 0), ChatMessageModel::GroupedRole).toBool(), true);
  QCOMPARE(model.data(model.index(2, 0), ChatMessageModel::GroupedRole).toBool(), false);
  QCOMPARE(model.data(model.index(2, 0), ChatMessageModel::OwnRole).toBool(), true);
  QCOMPARE(model.data(model.index(1, 0), ChatMessageModel::OwnRole).toBool(), false);
}

void TstChatModel::plainTextJoins() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  XmlMessage m1 = textMessage(QStringLiteral("first"));
  XmlMessage m2 = textMessage(QStringLiteral("second"));
  model.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  model.appendMessageLog(MT_Message, &jack, &jackName, &m2);
  QString log = model.plainText();
  QVERIFY(log.contains(QStringLiteral("first")));
  QVERIFY(log.contains(QStringLiteral("second")));
  QVERIFY(log.indexOf(QStringLiteral("first")) < log.indexOf(QStringLiteral("second")));
}

void TstChatModel::fileFlow() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  XmlMessage f = fileRequest(QStringLiteral("fid1"), QStringLiteral("test_file.txt"));
  QCOMPARE(model.appendMessageLog(MT_File, &jack, &jackName, &f), 0);
  QVERIFY(model.plainText().contains(QStringLiteral("sends you a file:")));
  QVERIFY(model.plainText().contains(QStringLiteral("test_file.txt")));
  model.updateFileMessage(FM_Receive, FO_Accept, QStringLiteral("fid1"));
  QVERIFY(model.plainText().contains(QStringLiteral("Accepted")));
  model.updateFileMessage(FM_Receive, FO_Complete, QStringLiteral("fid1"));
  QVERIFY(model.plainText().contains(QStringLiteral("Completed")));
}

void TstChatModel::folderKeepsType() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  XmlMessage f = fileRequest(QStringLiteral("fid9"), QStringLiteral("photos"));
  QCOMPARE(model.appendMessageLog(MT_Folder, &jack, &jackName, &f), 0);
  QCOMPARE(model.data(model.index(0, 0), ChatMessageModel::TypeRole).toInt(),
           int(MT_Folder));
  QVERIFY(model.plainText().contains(QStringLiteral("sends you a folder:")));
  model.updateFileMessage(FM_Receive, FO_Accept, QStringLiteral("fid9"));
  QVERIFY(model.plainText().contains(QStringLiteral("Accepted")));
}

void TstChatModel::typingRowShowsAndClears() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");

  XmlMessage composing = chatState(ChatStateNames[CS_Composing]);
  QCOMPARE(model.appendMessageLog(MT_ChatState, &jack, &jackName, &composing), 0);
  QCOMPARE(model.rowCount(), 1);
  QVERIFY(model.isTypingRow(0));
  QCOMPARE(model.data(model.index(0, 0), ChatMessageModel::TypeRole).toInt(),
           int(MT_ChatState));
  QCOMPARE(model.data(model.index(0, 0), ChatMessageModel::UserNameRole).toString(),
           jackName);
  QCOMPARE(model.data(model.index(0, 0), ChatMessageModel::GroupedRole).toBool(), false);
  QVERIFY(model.messageText(model.messageAt(0)).contains(jackName));
  QVERIFY(model.messageText(model.messageAt(0)).contains(QStringLiteral("typing")));

  //  Repeated composing reuses the row, no duplicate.
  QCOMPARE(model.appendMessageLog(MT_ChatState, &jack, &jackName, &composing), 0);
  QCOMPARE(model.rowCount(), 1);

  //  Any real message drops the typing row first.
  XmlMessage m = textMessage(QStringLiteral("hi"));
  QCOMPARE(model.appendMessageLog(MT_Message, &jack, &jackName, &m), 0);
  QCOMPARE(model.rowCount(), 1);
  QVERIFY(!model.isTypingRow(0));
  QCOMPARE(model.data(model.index(0, 0), ChatMessageModel::TypeRole).toInt(),
           int(MT_Message));
}

void TstChatModel::typingPausedText() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");

  XmlMessage paused = chatState(ChatStateNames[CS_Paused]);
  model.appendMessageLog(MT_ChatState, &jack, &jackName, &paused);
  QCOMPARE(model.rowCount(), 1);
  QVERIFY(model.messageText(model.messageAt(0)).contains(QStringLiteral("entered text")));

  //  State flip on the same row: composing replaces paused.
  XmlMessage composing = chatState(ChatStateNames[CS_Composing]);
  QCOMPARE(model.appendMessageLog(MT_ChatState, &jack, &jackName, &composing), 0);
  QCOMPARE(model.rowCount(), 1);
  QVERIFY(model.messageText(model.messageAt(0)).contains(QStringLiteral("typing")));
}

void TstChatModel::typingActiveClears() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");

  XmlMessage composing = chatState(ChatStateNames[CS_Composing]);
  model.appendMessageLog(MT_ChatState, &jack, &jackName, &composing);
  QCOMPARE(model.rowCount(), 1);
  XmlMessage active = chatState(ChatStateNames[CS_Active]);
  QCOMPARE(model.appendMessageLog(MT_ChatState, &jack, &jackName, &active), -1);
  QCOMPARE(model.rowCount(), 0);

  //  Unknown/blank state never shows a row.
  XmlMessage blank = chatState(QString());
  QCOMPARE(model.appendMessageLog(MT_ChatState, &jack, &jackName, &blank), -1);
  QCOMPARE(model.rowCount(), 0);
}

void TstChatModel::typingNotStored() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  model.localId = QStringLiteral("me");

  XmlMessage composing = chatState(ChatStateNames[CS_Composing]);
  model.appendMessageLog(MT_ChatState, &jack, &jackName, &composing);
  QVERIFY(model.messages().isEmpty());
  QVERIFY(model.plainText().isEmpty());
  QCOMPARE(model.data(model.index(0, 0), ChatMessageModel::OwnRole).toBool(), false);

  model.clearLog();
  QCOMPARE(model.rowCount(), 0);
}

void TstChatModel::typingDoesNotGroup() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");

  XmlMessage m1 = textMessage(QStringLiteral("hi"));
  model.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  XmlMessage composing = chatState(ChatStateNames[CS_Composing]);
  model.appendMessageLog(MT_ChatState, &jack, &jackName, &composing);
  //  Typing row is ephemeral: the next real message still groups.
  XmlMessage m2 = textMessage(QStringLiteral("again"));
  QCOMPARE(model.appendMessageLog(MT_Message, &jack, &jackName, &m2), 1);
  QCOMPARE(model.data(model.index(1, 0), ChatMessageModel::GroupedRole).toBool(), true);
}

void TstChatModel::textSigStable() {
  ChatMessageModel model;
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");

  XmlMessage m1 = textMessage(QStringLiteral("hi there"));
  model.appendMessageLog(MT_Message, &jack, &jackName, &m1);
  int len = 0;
  size_t h1 = model.rowTextHash(0, &len);
  QCOMPARE(len, int(QStringLiteral("hi there").length()));
  //  Second call serves the cache; non-text mutations keep the signature.
  QCOMPARE(model.rowTextHash(0), h1);
  QString newName = QStringLiteral("Jackson");
  model.updateUserName(&jack, &newName);
  QCOMPARE(model.rowTextHash(0), h1);

  XmlMessage f = fileRequest(QStringLiteral("fid1"), QStringLiteral("a.txt"));
  model.appendMessageLog(MT_File, &jack, &jackName, &f);
  size_t fh1 = model.rowTextHash(1);
  model.updateFileMessage(FM_Receive, FO_Accept, QStringLiteral("fid1"));
  QCOMPARE(model.rowTextHash(1), fh1);

  //  Distinct texts give distinct signatures; out-of-range is 0.
  XmlMessage m2 = textMessage(QStringLiteral("something else entirely"));
  model.appendMessageLog(MT_Message, &jack, &jackName, &m2);
  int len2 = 0;
  size_t h2 = model.rowTextHash(2, &len2);
  QCOMPARE(len2, int(QStringLiteral("something else entirely").length()));
  QVERIFY(h2 != h1 || len2 != len);
  QCOMPARE(model.rowTextHash(99), size_t(0));

  //  Typing states share the row but render different text: the signature must differ.
  ChatMessageModel typing;
  XmlMessage composing = chatState(ChatStateNames[CS_Composing]);
  QCOMPARE(typing.appendMessageLog(MT_ChatState, &jack, &jackName, &composing), 0);
  int clen = 0;
  size_t ch = typing.rowTextHash(0, &clen);
  XmlMessage paused = chatState(ChatStateNames[CS_Paused]);
  QCOMPARE(typing.appendMessageLog(MT_ChatState, &jack, &jackName, &paused), 0);
  int plen = 0;
  size_t ph = typing.rowTextHash(0, &plen);
  QVERIFY(ch != ph);
  QVERIFY(clen != plen);
}

QTEST_MAIN(TstChatModel)
#include "tst_chatmodel.moc"
