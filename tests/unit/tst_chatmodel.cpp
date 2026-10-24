// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Unit tests for ChatMessageModel (grouping, plainText, files).

#include <QtTest>

#include "core/definitions.h"
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

} // namespace

class TstChatModel : public QObject {
  Q_OBJECT

private slots:
  void grouping();
  void plainTextJoins();
  void fileFlow();
  void folderKeepsType();
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

QTEST_MAIN(TstChatModel)
#include "tst_chatmodel.moc"
