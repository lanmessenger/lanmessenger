// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.

#include <QtTest>

#include <QDateTime>
#include <QDir>
#include <QFile>

#include "core/settings.h"
#include "gui/chat/chatmessageview.h"
#include "messaging/history.h"
#include "messaging/xmlmessage.h"

namespace {

const QString kBlob = QStringLiteral(
  "<html><head><style type='text/css'>"
  "*{font-size: 9pt;} body {-webkit-nbsp-mode: space; word-wrap: break-word;}"
  "span.salutation {float:left; font-weight: bold;} span.time {float: right;}"
  "span.message {clear: both; display: block;} p {border-bottom: 1px solid #CCC;}"
  "</style></head><body><p><span class='salutation'>Jack:</span>"
  "<span class='time'>12:00</span>"
  "<span class='message'>hi there</span></p></body></html>");

bool writeQt6BuggyFile(const QString& path, const QString& user, const QDateTime& date,
                       const QString& html) {
  QFile::remove(path);
  if(!QDir().mkpath(QFileInfo(path).absolutePath()))
    return false;
  QFile file(path);
  if(!file.open(QIODevice::WriteOnly))
    return false;

  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);

  const qint64 dataPos = 8 + 4 + 2 + 4 + 8 + 8;
  const QByteArray payload = html.toUtf8();
  const qint64 indexPos = dataPos + 8 + 8 + 4 + payload.size();

  stream << QString(HC_DBMARKER);
  stream << HC_VERSION;
  stream << static_cast<qint16>(HC_HDRSIZE);
  stream << 1;
  stream << indexPos;
  stream << indexPos;

  if(stream.device()->pos() != dataPos)
    return false;
  stream << QString(HC_DTMARKER);
  stream << static_cast<qint64>(payload.size());
  stream << payload;

  if(stream.device()->pos() != indexPos)
    return false;
  stream << QString(HC_IDMARKER);
  stream << static_cast<qint64>(0);
  stream << dataPos;
  stream << date.toMSecsSinceEpoch();
  stream << user;
  return true;
}

} // namespace

class TstHistory : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void saveReturnsExactBlob();
  void savedSessionShowsAsNonEmptyRow();
  void qt6BuggyRecordStillReads();
  void structuredHistoryShowsOneRowPerMessage();
  void structuredHistoryKeepsEmojiRuns();
  void unstructuredBlobFallsBackToSingleRow();
  void nbspNormalizesToSpace();

private:
  QString scratchPath;
  QVariant oldSysPath;
  QVariant oldPath;
  bool hadSysPath = false;
  bool hadPath = false;
};

void TstHistory::initTestCase() {
  lmcSettings settings;
  hadSysPath = settings.contains(IDS_SYSHISTORYPATH);
  if(hadSysPath)
    oldSysPath = settings.value(IDS_SYSHISTORYPATH);
  hadPath = settings.contains(IDS_HISTORYPATH);
  if(hadPath)
    oldPath = settings.value(IDS_HISTORYPATH);
  scratchPath = QDir::tempPath() +
                QStringLiteral("/lmc-tst-history-") +
                QString::number(QCoreApplication::applicationPid());
  settings.setValue(IDS_SYSHISTORYPATH, false);
  settings.setValue(IDS_HISTORYPATH, scratchPath + QStringLiteral("/messenger.db"));
}

void TstHistory::cleanupTestCase() {
  lmcSettings settings;
  if(hadSysPath)
    settings.setValue(IDS_SYSHISTORYPATH, oldSysPath);
  else
    settings.remove(IDS_SYSHISTORYPATH);
  if(hadPath)
    settings.setValue(IDS_HISTORYPATH, oldPath);
  else
    settings.remove(IDS_HISTORYPATH);
  QDir(scratchPath).removeRecursively();
}

void TstHistory::saveReturnsExactBlob() {
  QFile::remove(History::historyFile());
  QString user = QStringLiteral("Jack");
  QDateTime when = QDateTime::currentDateTime();
  QString html = kBlob;
  QCOMPARE(History::save(user, when, &html), 0);

  QString user2 = QStringLiteral("Group Conversation");
  QString html2 = kBlob + QStringLiteral("second");
  QCOMPARE(History::save(user2, when, &html2), 0);

  QList<MsgInfo> list = History::getList();
  QCOMPARE(list.count(), 2);
  QCOMPARE(History::getMessage(list.at(0).offset), html);
  QCOMPARE(History::getMessage(list.at(1).offset), html2);
}

void TstHistory::savedSessionShowsAsNonEmptyRow() {
  QFile::remove(History::historyFile());
  QString user = QStringLiteral("Jack");
  QDateTime when = QDateTime::currentDateTime();
  QString html = kBlob;
  QCOMPARE(History::save(user, when, &html), 0);

  QList<MsgInfo> list = History::getList();
  QCOMPARE(list.count(), 1);
  QString blob = History::getMessage(list.first().offset);
  QCOMPARE(blob, html);

  ChatMessageView view;
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));
  view.initChat();

  QString name = list.first().name;
  QDateTime date = list.first().date;
  view.showLegacyHtml(&name, &date, &blob);
  QCOMPARE(view.rowCount(), 1);
  QVERIFY(view.messageAt(0).contains(QStringLiteral("hi there")));
  QVERIFY(!view.plainText().isEmpty());
  QVERIFY(!view.isOwnRow(0));
}

void TstHistory::qt6BuggyRecordStillReads() {
  QFile::remove(History::historyFile());
  QString user = QStringLiteral("Jack");
  QDateTime when = QDateTime::fromMSecsSinceEpoch(1760960000000);
  QVERIFY(writeQt6BuggyFile(History::historyFile(), user, when, kBlob));

  QList<MsgInfo> list = History::getList();
  QCOMPARE(list.count(), 1);
  QCOMPARE(list.first().name, user);
  QCOMPARE(History::getMessage(list.first().offset), kBlob);
}

void TstHistory::structuredHistoryShowsOneRowPerMessage() {
  ChatMessageView src;
  src.resize(400, 300);
  src.show();
  QVERIFY(QTest::qWaitForWindowExposed(&src, 5000));
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  QString jack = QStringLiteral("jack");
  QString jackName = QStringLiteral("Jack");
  src.localId = me;
  src.initChat();

  const qint64 t1 = 1761300000000;
  const qint64 t2 = 1761300060000;
  XmlMessage m1;
  m1.addHeader(XN_TIME, QString::number(t1));
  m1.addData(XN_MESSAGE, QStringLiteral("ping https://example.com/x"));
  XmlMessage m2;
  m2.addHeader(XN_TIME, QString::number(t2));
  m2.addData(XN_MESSAGE, QStringLiteral("line1\nline2"));
  src.appendMessageLog(MT_Message, &me, &meName, &m1);
  src.appendMessageLog(MT_Message, &jack, &jackName, &m2);

  QString blob = src.prepareMessageLogForSave();
  QVERIFY(ChatMessageView::isStructuredHistory(&blob));
  QVERIFY(!ChatMessageView::isStructuredHistory(&kBlob));

  ChatMessageView view;
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));
  view.localId = ChatMessageView::historyLocalId();
  view.initChat();

  QString name = QStringLiteral("Jack");
  QDateTime when = QDateTime::currentDateTime();
  view.showHistoryMessages(&name, &when, &blob);
  QCOMPARE(view.rowCount(), 2);
  //  Own message right, peer message left, bodies and order intact.
  QVERIFY(view.isOwnRow(0));
  QVERIFY(!view.isOwnRow(1));
  //  Saved &nbsp; must read back as plain spaces, not U+00A0.
  const QString first = view.messageAt(0);
  const QString second = view.messageAt(1);
  QVERIFY(!first.contains(QChar(0x00A0)));
  QVERIFY(!second.contains(QChar(0x00A0)));
  QCOMPARE(first, QStringLiteral("ping https://example.com/x"));
  QCOMPARE(second, QStringLiteral("line1\nline2"));
}

void TstHistory::structuredHistoryKeepsEmojiRuns() {
  ChatMessageView src;
  src.resize(400, 300);
  src.show();
  QVERIFY(QTest::qWaitForWindowExposed(&src, 5000));
  QString me = QStringLiteral("me");
  QString meName = QStringLiteral("Me");
  src.localId = me;
  src.initChat();

  //  S1 wraps emoji runs in nested <span> inside <span class='message'>:
  //  the loader must not truncate the text after the first </span>.
  const qint64 t1 = 1761300000000;
  XmlMessage m1;
  m1.addHeader(XN_TIME, QString::number(t1));
  m1.addData(XN_MESSAGE, QStringLiteral("hi \U0001F600 bye"));
  XmlMessage m2;
  m2.addHeader(XN_TIME, QString::number(t1 + 60000));
  m2.addData(XN_MESSAGE, QStringLiteral("\U0001F984\u200D\U0001F308!"));
  XmlMessage m3;
  m3.addHeader(XN_TIME, QString::number(t1 + 120000));
  m3.addData(XN_MESSAGE, QStringLiteral("go www.x/\U0001F600/end"));
  src.appendMessageLog(MT_Message, &me, &meName, &m1);
  src.appendMessageLog(MT_Message, &me, &meName, &m2);
  src.appendMessageLog(MT_Message, &me, &meName, &m3);

  QString blob = src.prepareMessageLogForSave();
  QVERIFY(ChatMessageView::isStructuredHistory(&blob));
  QVERIFY(blob.contains(QStringLiteral("Noto Color Emoji")));

  ChatMessageView view;
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));
  view.localId = ChatMessageView::historyLocalId();
  view.initChat();

  QString name = QStringLiteral("Jack");
  QDateTime when = QDateTime::currentDateTime();
  view.showHistoryMessages(&name, &when, &blob);
  QCOMPARE(view.rowCount(), 3);
  QCOMPARE(view.messageAt(0), QStringLiteral("hi \U0001F600 bye"));
  QCOMPARE(view.messageAt(1), QStringLiteral("\U0001F984\u200D\U0001F308!"));
  QCOMPARE(view.messageAt(2), QStringLiteral("go www.x/\U0001F600/end"));
}

void TstHistory::unstructuredBlobFallsBackToSingleRow() {
  ChatMessageView view;
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));
  view.localId = ChatMessageView::historyLocalId();
  view.initChat();

  QString name = QStringLiteral("Jack");
  QDateTime when = QDateTime::currentDateTime();
  QString legacy = kBlob;
  view.showHistoryMessages(&name, &when, &legacy);
  QCOMPARE(view.rowCount(), 1);
  QVERIFY(view.messageAt(0).contains(QStringLiteral("hi there")));
  QVERIFY(!view.isOwnRow(0));
}

void TstHistory::nbspNormalizesToSpace() {
  ChatMessageView view;
  view.resize(400, 300);
  view.show();
  QVERIFY(QTest::qWaitForWindowExposed(&view, 5000));
  view.localId = ChatMessageView::historyLocalId();
  view.initChat();

  QString name = QStringLiteral("Jack");
  QDateTime when = QDateTime::currentDateTime();
  QString legacy = kBlob;
  legacy.replace(QStringLiteral("hi there"), QStringLiteral("hi&nbsp;there"));
  view.showLegacyHtml(&name, &when, &legacy);
  QCOMPARE(view.rowCount(), 1);
  //  Legacy path keeps the whole blob as one row; &nbsp; still reads
  //  back as a plain space, not U+00A0.
  const QString body = view.messageAt(0);
  QVERIFY(!body.contains(QChar(0x00A0)));
  QCOMPARE(body, QStringLiteral("Jack:12:00hi there"));
}

QTEST_MAIN(TstHistory)
#include "tst_history.moc"
