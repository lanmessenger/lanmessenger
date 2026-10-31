// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.

#include <QtTest>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSignalSpy>

#include "core/definitions.h"
#include "core/settings.h"
#include "gui/files/filemodelview.h"
#include "legacy/history.h"
#include "storage/historystore.h"

namespace {

QString tempRoot() {
  return QDir::tempPath() + QStringLiteral("/lmc-tst-historystore-") +
         QString::number(QCoreApplication::applicationPid());
}

bool writeLegacyFile(const QString& path, const QString& user, const QDateTime& date,
                     const QString& html, bool buggy64) {
  QFile::remove(path);
  if(!QDir().mkpath(QFileInfo(path).absolutePath()))
    return false;
  QFile file(path);
  if(!file.open(QIODevice::WriteOnly))
    return false;
  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);
  const QByteArray payload = html.toUtf8();
  const qint64 dataPos = 8 + 4 + 2 + 4 + 8 + 8;
  const qint64 indexPos = dataPos + 8 + (buggy64 ? 8 : 4) + 4 + payload.size();
  stream << QString(HC_DBMARKER);
  stream << HC_VERSION;
  stream << static_cast<qint16>(HC_HDRSIZE);
  stream << 1;
  stream << indexPos;
  stream << indexPos;
  if(stream.device()->pos() != dataPos)
    return false;
  stream << QString(HC_DTMARKER);
  if(buggy64)
    stream << static_cast<qint64>(payload.size());
  else
    stream << static_cast<qint32>(payload.size());
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

QString structuredBlob(qint64 t1, qint64 t2) {
  return QStringLiteral("<html data-lmc-history=\"2\"><head></head><body>"
                        "<p data-own=\"1\" data-ts=\"%1\" data-type=\"%3\">"
                        "<span class='salutation'>Me:</span>"
                        "<span class='time'>12:00</span>"
                        "<span class='message'>hi \U0001F600</span></p>"
                        "<p data-own=\"0\" data-ts=\"%2\" data-type=\"%3\">"
                        "<span class='salutation'>Jack:</span>"
                        "<span class='time'>12:01</span>"
                        "<span class='message'>hello</span></p>"
                        "</body></html>")
    .arg(t1)
    .arg(t2)
    .arg(int(MT_Message));
}

const QString kLegacyHtml = QStringLiteral(
  "<html><head></head><body><p><span class='salutation'>Jack:</span>"
  "<span class='time'>12:00</span>"
  "<span class='message'>hi there</span></p></body></html>");

bool openStore(const QString& dbPath) {
  HistoryStore::resetForTests();
  HistoryStore::setDatabasePathForTests(dbPath);
  return HistoryStore::instance()->open();
}

} // namespace

class TstHistoryStore : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void init();
  void cleanup();
  void roundtripSessionRows();
  void clearSessionsEmits();
  void transfersRoundtrip();
  void legacyStructuredImport();
  void legacyQt6BuggyImport();
  void legacyUnstructuredFallsBackToSingleRow();
  void legacyTransfersImport();
  void importIdempotent();
  void legacySkippedWhenDbNotEmpty();
  void emptySessionStillStored();
  void failedMkpathRestoresPath();
  void backupPathIsCollisionFree();

private:
  QString root;
  QString dbPath;
};

void TstHistoryStore::initTestCase() {
  root = tempRoot();
  QDir(root).removeRecursively();
  QDir().mkpath(root);
}

void TstHistoryStore::cleanupTestCase() {
  HistoryStore::resetForTests();
  QDir(root).removeRecursively();
}

void TstHistoryStore::init() {
  // Monotonic seq: two cases in the same millisecond must not share a path.
  static int seq = 0;
  dbPath = root + QStringLiteral("/case-%1-%2-%3.lanmessenger.db")
                    .arg(QString::number(QDateTime::currentMSecsSinceEpoch()),
                         QString::number((quintptr)this), QString::number(++seq));
  QFile::remove(dbPath);
  QDir().mkpath(QFileInfo(dbPath).absolutePath());
  QVERIFY(openStore(dbPath));
}

void TstHistoryStore::cleanup() {
  HistoryStore::instance()->close();
  HistoryStore::resetForTests();
  QFile::remove(dbPath);
}

void TstHistoryStore::roundtripSessionRows() {
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy spy(store, &HistoryStore::sessionsChanged);
  QList<StoredMessage> rows;
  StoredMessage m1;
  m1.senderId = QStringLiteral("me");
  m1.senderName = QStringLiteral("Me");
  m1.isOwn = true;
  m1.messageType = int(MT_Message);
  m1.createdAt = 1761300000000;
  m1.body = QStringLiteral("hi \U0001F600 bye");
  StoredMessage m2;
  m2.senderId = QStringLiteral("jack");
  m2.senderName = QStringLiteral("Jack");
  m2.isOwn = false;
  m2.messageType = int(MT_Message);
  m2.createdAt = 1761300060000;
  m2.body = QStringLiteral("hello");
  StoredMessage m3;
  m3.senderId = QStringLiteral("me");
  m3.senderName = QStringLiteral("Me");
  m3.isOwn = true;
  m3.messageType = int(MT_GroupMessage);
  m3.createdAt = 1761300120000;
  m3.body = QStringLiteral("group hi");
  rows << m1 << m2 << m3;
  QVERIFY(store->saveSession(QStringLiteral("Jack"), QDateTime::currentDateTime(), rows));
  QCOMPARE(spy.count(), 1);

  QList<SessionInfo> sessions = store->listSessions();
  QCOMPARE(sessions.count(), 1);
  QCOMPARE(sessions.first().name, QStringLiteral("Jack"));
  QList<StoredMessage> back = store->messagesForSession(sessions.first().id);
  QCOMPARE(back.count(), 3);
  QCOMPARE(back.at(0).body, m1.body);
  QVERIFY(back.at(0).isOwn);
  QVERIFY(!back.at(1).isOwn);
  QCOMPARE(back.at(1).senderName, QStringLiteral("Jack"));
  QCOMPARE(back.at(2).messageType, int(MT_GroupMessage));
  QCOMPARE(back.at(0).createdAt, m1.createdAt);
}

void TstHistoryStore::clearSessionsEmits() {
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy spy(store, &HistoryStore::sessionsChanged);
  StoredMessage m;
  m.senderId = m.senderName = QStringLiteral("Jack");
  m.messageType = int(MT_Message);
  m.createdAt = 1761300000000;
  m.body = QStringLiteral("hi");
  QVERIFY(store->saveSession(QStringLiteral("Jack"), QDateTime::currentDateTime(), {m}));
  QCOMPARE(store->listSessions().count(), 1);
  QVERIFY(store->clearSessions());
  QVERIFY(store->listSessions().isEmpty());
  QVERIFY(store->messagesForSession(1).isEmpty());
  QCOMPARE(spy.count(), 2);
}

void TstHistoryStore::transfersRoundtrip() {
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy spy(store, &HistoryStore::transfersChanged);
  TransferRecord r1;
  r1.id = QStringLiteral("id-1");
  r1.mode = 0;
  r1.filePath = QStringLiteral("/tmp/a.txt");
  r1.fileDisplay = QStringLiteral("a.txt (1 KB)");
  r1.userId = QStringLiteral("u1");
  r1.userName = QStringLiteral("Jack");
  r1.fileSize = 1024;
  r1.state = 4;
  r1.createdAt = 1761300000000;
  TransferRecord r2 = r1;
  r2.id = QStringLiteral("id-2");
  r2.mode = 1;
  QVERIFY(store->saveTransfers({r1, r2}));
  QCOMPARE(spy.count(), 1);
  QList<TransferRecord> back = store->loadTransfers();
  QCOMPARE(back.count(), 2);
  QCOMPARE(back.at(0).id, r1.id);
  QCOMPARE(back.at(0).userName, QStringLiteral("Jack"));
  QCOMPARE(back.at(0).fileSize, 1024);
  QVERIFY(store->removeTransfer(QStringLiteral("id-1"), 0));
  QCOMPARE(store->loadTransfers().count(), 1);
  // Same id in the other mode is a distinct row: add, remove by (id, mode).
  TransferRecord dup = r2;
  dup.id = QStringLiteral("id-2");
  dup.mode = 0;
  dup.fileDisplay = QStringLiteral("a.txt (1 KB) dup");
  {
    QList<TransferRecord> rows = store->loadTransfers();
    rows.append(dup);
    QVERIFY(store->saveTransfers(rows));
  }
  QCOMPARE(store->loadTransfers().count(), 2);
  QVERIFY(store->removeTransfer(QStringLiteral("id-2"), 0));
  QCOMPARE(store->loadTransfers().count(), 1);
  QCOMPARE(store->loadTransfers().first().mode, 1);
  QVERIFY(store->clearTransfers());
  QVERIFY(store->loadTransfers().isEmpty());
}

void TstHistoryStore::legacyStructuredImport() {
  QString legacyPath = root + QStringLiteral("/structured.db");
  qint64 t1 = 1761300000000;
  qint64 t2 = 1761300060000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t2),
                          false));
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy spy(store, &HistoryStore::sessionsChanged);
  QVERIFY(store->importLegacyFrom(legacyPath, QString()));
  QVERIFY(spy.count() >= 1);
  QList<SessionInfo> sessions = store->listSessions();
  QCOMPARE(sessions.count(), 1);
  QList<StoredMessage> rows = store->messagesForSession(sessions.first().id);
  QCOMPARE(rows.count(), 2);
  QVERIFY(rows.at(0).isOwn);
  QVERIFY(!rows.at(1).isOwn);
  QVERIFY(rows.at(0).body.contains(QStringLiteral("\U0001F600")));
  QCOMPARE(rows.at(1).body, QStringLiteral("hello"));
  QVERIFY(QFile::exists(legacyPath + QStringLiteral(".bak")));
}

void TstHistoryStore::legacyQt6BuggyImport() {
  QString legacyPath = root + QStringLiteral("/buggy.db");
  qint64 t1 = 1760960000000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          true));
  HistoryStore* store = HistoryStore::instance();
  QVERIFY(store->importLegacyFrom(legacyPath, QString()));
  QList<SessionInfo> sessions = store->listSessions();
  QCOMPARE(sessions.count(), 1);
  QCOMPARE(store->messagesForSession(sessions.first().id).count(), 2);
}

void TstHistoryStore::legacyUnstructuredFallsBackToSingleRow() {
  QString legacyPath = root + QStringLiteral("/legacy.db");
  qint64 t1 = 1760960000000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), kLegacyHtml, false));
  HistoryStore* store = HistoryStore::instance();
  QVERIFY(store->importLegacyFrom(legacyPath, QString()));
  QList<SessionInfo> sessions = store->listSessions();
  QCOMPARE(sessions.count(), 1);
  QList<StoredMessage> rows = store->messagesForSession(sessions.first().id);
  QCOMPARE(rows.count(), 1);
  QVERIFY(rows.first().body.contains(QStringLiteral("hi there")));
  QVERIFY(!rows.first().isOwn);
}

void TstHistoryStore::legacyTransfersImport() {
  QString lstPath = root + QStringLiteral("/transfers.lst");
  {
    FileModel model;
    FileView v(QStringLiteral("tid-1"));
    v.mode = FileView::TM_Send;
    v.filePath = QStringLiteral("/tmp/a.txt");
    v.userName = QStringLiteral("Jack");
    v.fileDisplay = QStringLiteral("a.txt (1 KB)");
    v.state = FileView::TS_Complete;
    v.icon = QPixmap();
    model.insertItem(0, &v);
    model.saveData(lstPath);
  }
  QVERIFY(QFile::exists(lstPath));
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy spy(store, &HistoryStore::transfersChanged);
  QVERIFY(store->importLegacyFrom(QString(), lstPath));
  QVERIFY(spy.count() >= 1);
  QList<TransferRecord> back = store->loadTransfers();
  QCOMPARE(back.count(), 1);
  QCOMPARE(back.first().id, QStringLiteral("tid-1"));
  QCOMPARE(back.first().userName, QStringLiteral("Jack"));
  // Legacy stream never stored userId/fileSize: no garbage allowed.
  QCOMPARE(back.first().userId, QStringLiteral("Jack"));
  QCOMPARE(back.first().fileSize, 0);
  QVERIFY(QFile::exists(lstPath + QStringLiteral(".bak")));
}

void TstHistoryStore::importIdempotent() {
  QString legacyPath = root + QStringLiteral("/idem.db");
  qint64 t1 = 1761300000000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          false));
  HistoryStore* store = HistoryStore::instance();
  QVERIFY(store->importLegacyFrom(legacyPath, QString()));
  QCOMPARE(store->listSessions().count(), 1);
  // Second import: .bak exists, source gone, non-empty DB -> no duplicates.
  QVERIFY(store->importLegacyFrom(legacyPath, QString()));
  QCOMPARE(store->listSessions().count(), 1);
  QCOMPARE(store->messagesForSession(store->listSessions().first().id).count(), 2);
}

void TstHistoryStore::legacySkippedWhenDbNotEmpty() {
  HistoryStore* store = HistoryStore::instance();
  StoredMessage m;
  m.senderId = m.senderName = QStringLiteral("Jack");
  m.messageType = int(MT_Message);
  m.createdAt = 1761300000000;
  m.body = QStringLiteral("hi");
  QVERIFY(store->saveSession(QStringLiteral("Jack"), QDateTime::currentDateTime(), {m}));
  QString legacyPath = root + QStringLiteral("/skip.db");
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Other"),
                          QDateTime::currentDateTime(), kLegacyHtml, false));
  QVERIFY(store->importLegacyFrom(legacyPath, QString()));
  // No duplicates, data untouched, source not renamed.
  QCOMPARE(store->listSessions().count(), 1);
  QCOMPARE(store->listSessions().first().name, QStringLiteral("Jack"));
  QVERIFY(QFile::exists(legacyPath));
}

void TstHistoryStore::emptySessionStillStored() {
  // File-only conversations (no text rows) are still stored, as before.
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy spy(store, &HistoryStore::sessionsChanged);
  QVERIFY(store->saveSession(QStringLiteral("Jack"), QDateTime::currentDateTime(), {}));
  QCOMPARE(spy.count(), 1);
  QList<SessionInfo> sessions = store->listSessions();
  QCOMPARE(sessions.count(), 1);
  QCOMPARE(sessions.first().name, QStringLiteral("Jack"));
  QVERIFY(store->messagesForSession(sessions.first().id).isEmpty());
}

void TstHistoryStore::failedMkpathRestoresPath() {
  // No test-path seam: a failed open must not strand m_path at the bad dir
  // while the old database stays open.
  HistoryStore::resetForTests();
  lmcSettings settings;
  const bool hadSys = settings.contains(IDS_SYSHISTORYPATH);
  const QVariant oldSys = settings.value(IDS_SYSHISTORYPATH);
  const bool hadPath = settings.contains(IDS_HISTORYPATH);
  const QVariant oldPath = settings.value(IDS_HISTORYPATH);
  settings.setValue(IDS_SYSHISTORYPATH, false);
  const QString good = root + QStringLiteral("/mkpath-good/lanmessenger.db");
  settings.setValue(IDS_HISTORYPATH, good);
  HistoryStore* store = HistoryStore::instance();
  QVERIFY(store->open());
  StoredMessage m1;
  m1.senderId = m1.senderName = QStringLiteral("Jack");
  m1.messageType = int(MT_Message);
  m1.createdAt = 1761300000000;
  m1.body = QStringLiteral("first");
  QVERIFY(store->saveSession(QStringLiteral("Jack"), QDateTime::currentDateTime(), {m1}));
  // A regular file as the parent component makes mkpath fail.
  const QString blocker = root + QStringLiteral("/mkpath-blocker");
  {
    QFile bf(blocker);
    QVERIFY(bf.open(QIODevice::WriteOnly));
    bf.write("x");
  }
  const QString bad = blocker + QStringLiteral("/x.db");
  settings.setValue(IDS_HISTORYPATH, bad);
  QVERIFY(!store->open(bad));
  // Must retry the (still failing) wanted path, not claim the stale open DB.
  QVERIFY(!store->reopenIfPathChanged());
  StoredMessage m2 = m1;
  m2.body = QStringLiteral("second");
  QVERIFY(!store->saveSession(QStringLiteral("Jack"), QDateTime::currentDateTime(), {m2}));
  settings.setValue(IDS_HISTORYPATH, good);
  QVERIFY(store->open());
  QCOMPARE(store->listSessions().count(), 1);
  if(hadSys)
    settings.setValue(IDS_SYSHISTORYPATH, oldSys);
  else
    settings.remove(IDS_SYSHISTORYPATH);
  if(hadPath)
    settings.setValue(IDS_HISTORYPATH, oldPath);
  else
    settings.remove(IDS_HISTORYPATH);
  QFile::remove(blocker);
  QDir(root + QStringLiteral("/mkpath-good")).removeRecursively();
}

void TstHistoryStore::backupPathIsCollisionFree() {
  const QString base = root + QStringLiteral("/r2-test.db.bak");
  QFile::remove(base);
  // Free base returns itself.
  QCOMPARE(HistoryStore::uniqueBackupPath(base), base);
  {
    QFile bf(base);
    QVERIFY(bf.open(QIODevice::WriteOnly));
    bf.write("x");
  }
  const QString stamped = HistoryStore::uniqueBackupPath(base);
  QVERIFY(stamped.startsWith(base + QStringLiteral(".")));
  QVERIFY(!QFile::exists(stamped));
  // Occupy the stamped name: the next call must differ, never overwrite.
  {
    QFile sf(stamped);
    QVERIFY(sf.open(QIODevice::WriteOnly));
    sf.write("y");
  }
  const QString second = HistoryStore::uniqueBackupPath(base);
  QVERIFY(!QFile::exists(second));
  QVERIFY(second != stamped);
  QVERIFY(second.startsWith(base + QStringLiteral(".")));
  QFile::remove(base);
  QFile::remove(stamped);
}

QTEST_MAIN(TstHistoryStore)
#include "tst_historystore.moc"
