// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.

#include <QtTest>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "core/definitions.h"
#include "storage/historystore.h"

namespace {

QString tempRoot() {
  return QDir::tempPath() + QStringLiteral("/lmc-tst-historystore-") +
         QString::number(QCoreApplication::applicationPid());
}

bool openStore(const QString& dbPath) {
  HistoryStore::resetForTests();
  return HistoryStore::instance()->open(dbPath);
}

int schemaVersionOfOpenStore() {
  // Same connection name as HistoryStore ("lmc_history"): SQL stays in storage + its tests.
  QSqlDatabase db = QSqlDatabase::database(QStringLiteral("lmc_history"));
  QSqlQuery q(db);
  if(!q.exec(QStringLiteral("PRAGMA user_version")))
    return -999;
  if(!q.next())
    return -999;
  return q.value(0).toInt();
}

bool setSchemaVersionOfOpenStore(int version) {
  QSqlDatabase db = QSqlDatabase::database(QStringLiteral("lmc_history"));
  QSqlQuery q(db);
  return q.exec(QStringLiteral("PRAGMA user_version = %1").arg(version));
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
  void saveSessionBatchStoresAllSessions();
  void saveSessionBatchEmptyIsNoop();
  void transfersRoundtrip();
  void emptySessionStillStored();
  void failedMkpathRestoresPath();
  void unsupportedVersionRestoresPath();
  void backupPathIsCollisionFree();
  void freshDbStampsSchemaVersion();
  void unstampedDevDbIsStampedWithoutDataLoss();
  void newerDbRefusesOpen();
  void openEmitsOnceOnSuccess();
  void reopenSamePathIsSilent();
  void openTwiceEmitsTwice();

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

void TstHistoryStore::saveSessionBatchStoresAllSessions() {
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy spy(store, &HistoryStore::sessionsChanged);
  StoredMessage m;
  m.senderId = m.senderName = QStringLiteral("Jack");
  m.messageType = int(MT_Message);
  m.createdAt = 1761300000000;
  m.body = QStringLiteral("hi");
  SessionToSave s1;
  s1.name = QStringLiteral("Jack");
  s1.closedAt = QDateTime::fromMSecsSinceEpoch(1761300000000);
  s1.messages = {m, m};
  SessionToSave s2;
  s2.name = QStringLiteral("Group");
  s2.closedAt = QDateTime::fromMSecsSinceEpoch(1761300100000);
  QVERIFY(store->saveSessionBatch({s1, s2}));
  QCOMPARE(spy.count(), 1);
  QList<SessionInfo> sessions = store->listSessions();
  QCOMPARE(sessions.count(), 2);
  int withRows = 0;
  int emptyRows = 0;
  for(const SessionInfo& s : sessions) {
    const int n = store->messagesForSession(s.id).count();
    if(s.name == QStringLiteral("Jack")) {
      QCOMPARE(n, 2);
      withRows++;
    } else {
      QCOMPARE(n, 0);
      emptyRows++;
    }
  }
  QCOMPARE(withRows, 1);
  QCOMPARE(emptyRows, 1);
}

void TstHistoryStore::saveSessionBatchEmptyIsNoop() {
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy spy(store, &HistoryStore::sessionsChanged);
  QVERIFY(store->saveSessionBatch({}));
  QCOMPARE(spy.count(), 0);
  QVERIFY(store->listSessions().isEmpty());
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
  // A failed open must not strand m_path at the bad dir
  // while the old database stays open.
  HistoryStore::resetForTests();
  const QString good = root + QStringLiteral("/mkpath-good/lanmessenger.db");
  HistoryStore* store = HistoryStore::instance();
  QVERIFY(store->open(good));
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
  QVERIFY(!store->open(bad));
  // Must retry the (still failing) wanted path, not claim the stale open DB.
  QVERIFY(!store->reopen(bad));
  StoredMessage m2 = m1;
  m2.body = QStringLiteral("second");
  QVERIFY(!store->saveSession(QStringLiteral("Jack"), QDateTime::currentDateTime(), {m2}));
  QVERIFY(store->reopen(good));
  QCOMPARE(store->listSessions().count(), 1);
  QFile::remove(blocker);
  QDir(root + QStringLiteral("/mkpath-good")).removeRecursively();
}

void TstHistoryStore::unsupportedVersionRestoresPath() {
  // Late failure (version gate) must restore bookkeeping like the mkpath one.
  HistoryStore::resetForTests();
  const QString good = root + QStringLiteral("/ver-good/lanmessenger.db");
  HistoryStore* store = HistoryStore::instance();
  QVERIFY(store->open(good));
  const QString newer = root + QStringLiteral("/ver-new/lanmessenger.db");
  QVERIFY(store->open(newer));
  QVERIFY(setSchemaVersionOfOpenStore(99));
  store->close();
  QVERIFY(store->open(good));
  QVERIFY(!store->open(newer));
  QVERIFY(!store->isOpen());
  QCOMPARE(store->databasePath(), good);
  QVERIFY(store->reopen(good));
  QVERIFY(store->isOpen());
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

void TstHistoryStore::freshDbStampsSchemaVersion() {
  QCOMPARE(schemaVersionOfOpenStore(), 1);
  // Reopen keeps the stamp.
  HistoryStore* store = HistoryStore::instance();
  store->close();
  QVERIFY(store->open(dbPath));
  QCOMPARE(schemaVersionOfOpenStore(), 1);
}

void TstHistoryStore::unstampedDevDbIsStampedWithoutDataLoss() {
  HistoryStore* store = HistoryStore::instance();
  StoredMessage m;
  m.senderId = m.senderName = QStringLiteral("Jack");
  m.messageType = int(MT_Message);
  m.createdAt = 1761300000000;
  m.body = QStringLiteral("hi");
  QVERIFY(store->saveSession(QStringLiteral("Jack"), QDateTime::currentDateTime(), {m}));
  // Simulate a pre-stamp dev DB: strip the version, keep the rows.
  QVERIFY(setSchemaVersionOfOpenStore(0));
  store->close();
  QVERIFY(store->open(dbPath));
  QCOMPARE(schemaVersionOfOpenStore(), 1);
  QCOMPARE(store->listSessions().count(), 1);
  QCOMPARE(store->messagesForSession(store->listSessions().first().id).count(), 1);
}

void TstHistoryStore::newerDbRefusesOpen() {
  QVERIFY(setSchemaVersionOfOpenStore(999));
  HistoryStore* store = HistoryStore::instance();
  store->close();
  QVERIFY(!store->open(dbPath));
  QVERIFY(!store->isOpen());
  QVERIFY(QFile::exists(dbPath));
}

void TstHistoryStore::openEmitsOnceOnSuccess() {
  HistoryStore* store = HistoryStore::instance();
  store->close();
  QSignalSpy sessionsSpy(store, &HistoryStore::sessionsChanged);
  QSignalSpy transfersSpy(store, &HistoryStore::transfersChanged);
  QVERIFY(store->open(dbPath));
  QCOMPARE(sessionsSpy.count(), 1);
  QCOMPARE(transfersSpy.count(), 1);
}

void TstHistoryStore::reopenSamePathIsSilent() {
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy sessionsSpy(store, &HistoryStore::sessionsChanged);
  QSignalSpy transfersSpy(store, &HistoryStore::transfersChanged);
  QVERIFY(store->reopen(dbPath));
  QVERIFY(sessionsSpy.isEmpty());
  QVERIFY(transfersSpy.isEmpty());
}

void TstHistoryStore::openTwiceEmitsTwice() {
  // open() is unconditional: no silent fast path inside, that lives in reopen().
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy sessionsSpy(store, &HistoryStore::sessionsChanged);
  QSignalSpy transfersSpy(store, &HistoryStore::transfersChanged);
  QVERIFY(store->open(dbPath));
  QVERIFY(store->open(dbPath));
  QCOMPARE(sessionsSpy.count(), 2);
  QCOMPARE(transfersSpy.count(), 2);
}

QTEST_MAIN(TstHistoryStore)
#include "tst_historystore.moc"
