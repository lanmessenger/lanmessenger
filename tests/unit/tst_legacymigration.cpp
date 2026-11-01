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
#include "core/settings.h"
#include "core/stdlocation.h"
#include "gui/files/filemodelview.h"
#include "legacy/history.h"
#include "legacy/legacymigration.h"
#include "storage/historystore.h"

namespace {

// Restores real lmcSettings on destruction (QVERIFY/QSKIP safe).
struct SettingsGuard {
  lmcSettings settings;
  bool hadSys;
  QVariant oldSys;
  bool hadPath;
  QVariant oldPath;
  SettingsGuard()
      : hadSys(settings.contains(IDS_SYSHISTORYPATH)),
        oldSys(settings.value(IDS_SYSHISTORYPATH)),
        hadPath(settings.contains(IDS_HISTORYPATH)),
        oldPath(settings.value(IDS_HISTORYPATH)) {
  }
  ~SettingsGuard() {
    if(hadSys)
      settings.setValue(IDS_SYSHISTORYPATH, oldSys);
    else
      settings.remove(IDS_SYSHISTORYPATH);
    if(hadPath)
      settings.setValue(IDS_HISTORYPATH, oldPath);
    else
      settings.remove(IDS_HISTORYPATH);
  }
};

QString tempRoot() {
  return QDir::tempPath() + QStringLiteral("/lmc-tst-legacymigration-") +
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
  return HistoryStore::instance()->open(dbPath);
}

} // namespace

class TstLegacyMigration : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void init();
  void cleanup();
  void structuredImport();
  void buggyImport();
  void unstructuredFallsBackToSingleRow();
  void transfersImport();
  void importIdempotent();
  void skippedWhenDbNotEmpty();
  void stashAtTargetPath();
  void failedMigrationRestoresStashForRetry();
  void partialFailureRestoresStashForRetry();
  void orphanStashPreserved();
  void restoreStashReportsFailure();
  void bakCollisionUsesUniqueName();
  void failedImportClosesStoreForRetry();
  void probeQuietWithoutSources();
  void probeFindsLegacyAtTarget();

private:
  QString root;
  QString dbPath;
};

void TstLegacyMigration::initTestCase() {
  root = tempRoot();
  QDir(root).removeRecursively();
  QDir().mkpath(root);
}

void TstLegacyMigration::cleanupTestCase() {
  HistoryStore::resetForTests();
  QDir(root).removeRecursively();
}

void TstLegacyMigration::init() {
  static int seq = 0;
  dbPath = root + QStringLiteral("/case-%1-%2-%3.lanmessenger.db")
                    .arg(QString::number(QDateTime::currentMSecsSinceEpoch()),
                         QString::number((quintptr)this), QString::number(++seq));
  QFile::remove(dbPath);
  QDir().mkpath(QFileInfo(dbPath).absolutePath());
  QVERIFY(openStore(dbPath));
}

void TstLegacyMigration::cleanup() {
  HistoryStore::instance()->close();
  HistoryStore::resetForTests();
  QFile::remove(dbPath);
}

void TstLegacyMigration::structuredImport() {
  QString legacyPath = root + QStringLiteral("/structured.db");
  qint64 t1 = 1761300000000;
  qint64 t2 = 1761300060000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t2),
                          false));
  HistoryStore* store = HistoryStore::instance();
  QSignalSpy spy(store, &HistoryStore::sessionsChanged);
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, legacyPath, QString()));
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

void TstLegacyMigration::buggyImport() {
  QString legacyPath = root + QStringLiteral("/buggy.db");
  qint64 t1 = 1760960000000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          true));
  HistoryStore* store = HistoryStore::instance();
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, legacyPath, QString()));
  QList<SessionInfo> sessions = store->listSessions();
  QCOMPARE(sessions.count(), 1);
  QCOMPARE(store->messagesForSession(sessions.first().id).count(), 2);
}

void TstLegacyMigration::unstructuredFallsBackToSingleRow() {
  QString legacyPath = root + QStringLiteral("/legacy.db");
  qint64 t1 = 1760960000000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), kLegacyHtml, false));
  HistoryStore* store = HistoryStore::instance();
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, legacyPath, QString()));
  QList<SessionInfo> sessions = store->listSessions();
  QCOMPARE(sessions.count(), 1);
  QList<StoredMessage> rows = store->messagesForSession(sessions.first().id);
  QCOMPARE(rows.count(), 1);
  QVERIFY(rows.first().body.contains(QStringLiteral("hi there")));
  QVERIFY(!rows.first().isOwn);
}

void TstLegacyMigration::transfersImport() {
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
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, QString(), lstPath));
  QVERIFY(spy.count() >= 1);
  QList<TransferRecord> back = store->loadTransfers();
  QCOMPARE(back.count(), 1);
  QCOMPARE(back.first().id, QStringLiteral("tid-1"));
  QCOMPARE(back.first().userName, QStringLiteral("Jack"));
  QCOMPARE(back.first().userId, QStringLiteral("Jack"));
  QCOMPARE(back.first().fileSize, 0);
  QVERIFY(QFile::exists(lstPath + QStringLiteral(".bak")));
}

void TstLegacyMigration::importIdempotent() {
  QString legacyPath = root + QStringLiteral("/idem.db");
  qint64 t1 = 1761300000000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          false));
  HistoryStore* store = HistoryStore::instance();
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, legacyPath, QString()));
  QCOMPARE(store->listSessions().count(), 1);
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, legacyPath, QString()));
  QCOMPARE(store->listSessions().count(), 1);
  QCOMPARE(store->messagesForSession(store->listSessions().first().id).count(), 2);
}

void TstLegacyMigration::skippedWhenDbNotEmpty() {
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
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, legacyPath, QString()));
  QCOMPARE(store->listSessions().count(), 1);
  QCOMPARE(store->listSessions().first().name, QStringLiteral("Jack"));
  // Skipped source is left in place: a later clearSessions re-imports it.
  QVERIFY(QFile::exists(legacyPath));
  QVERIFY(!QFile::exists(legacyPath + QStringLiteral(".bak")));
}

void TstLegacyMigration::stashAtTargetPath() {
  HistoryStore* store = HistoryStore::instance();
  store->close();
  qint64 t1 = 1761300000000;
  QVERIFY(writeLegacyFile(dbPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          false));
  QString stashed;
  QVERIFY(LegacyMigration::stashLegacyAtTarget(dbPath, &stashed));
  QVERIFY(!stashed.isEmpty());
  QVERIFY(!QFile::exists(dbPath));
  QVERIFY(store->open(dbPath));
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, stashed, QString()));
  LegacyMigration::preserveStashLeftovers(dbPath, stashed);
  QCOMPARE(store->listSessions().count(), 1);
  QCOMPARE(store->messagesForSession(store->listSessions().first().id).count(), 2);
  QVERIFY(!QFile::exists(stashed));
  QVERIFY(QFile::exists(dbPath + QStringLiteral(".bak")));
}

void TstLegacyMigration::failedMigrationRestoresStashForRetry() {
  HistoryStore* store = HistoryStore::instance();
  store->close();
  qint64 t1 = 1761300000000;
  QVERIFY(writeLegacyFile(dbPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          false));
  QString stashed;
  QVERIFY(LegacyMigration::stashLegacyAtTarget(dbPath, &stashed));
  QVERIFY(!stashed.isEmpty());
  // Failed import: source back at target, probe finds work again.
  QVERIFY(LegacyMigration::settleStash(dbPath, stashed, false));
  QVERIFY(QFile::exists(dbPath));
  QVERIFY(!QFile::exists(dbPath + QStringLiteral(".bak")));
  QString h;
  QString t;
  QVERIFY(LegacyMigration::hasLegacyWork(dbPath, &h, &t));
  QCOMPARE(h, dbPath);
  // Successful import settles the stash to .bak instead.
  QString stashed2;
  QVERIFY(LegacyMigration::stashLegacyAtTarget(dbPath, &stashed2));
  QVERIFY(!stashed2.isEmpty());
  QVERIFY(LegacyMigration::settleStash(dbPath, stashed2, true));
  QVERIFY(!QFile::exists(stashed2));
  QVERIFY(QFile::exists(dbPath + QStringLiteral(".bak")));
}

void TstLegacyMigration::partialFailureRestoresStashForRetry() {
  HistoryStore* store = HistoryStore::instance();
  store->close();
  qint64 t1 = 1761300000000;
  QVERIFY(writeLegacyFile(dbPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          false));
  const QString lstPath = root + QStringLiteral("/partial.lst");
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
  QString stashed;
  QVERIFY(LegacyMigration::stashLegacyAtTarget(dbPath, &stashed));
  QVERIFY(!stashed.isEmpty());
  QVERIFY(store->open(dbPath));
  // Sabotage the second phase: sessions commit, transfers fail.
  {
    // Same connection name as HistoryStore ("lmc_history").
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("lmc_history"));
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("DROP TABLE transfers")));
  }
  QVERIFY(!LegacyMigration::migrateLegacyHistory(*store, stashed, lstPath));
  // Nothing acknowledged: no .bak anywhere, stash restored for retry.
  QVERIFY(!QFile::exists(stashed + QStringLiteral(".bak")));
  QVERIFY(!QFile::exists(lstPath + QStringLiteral(".bak")));
  QVERIFY(!QFile::exists(dbPath + QStringLiteral(".bak")));
  QVERIFY(LegacyMigration::settleStash(dbPath, stashed, false));
  QVERIFY(History::hasDbMarker(dbPath));
  QString h;
  QString t;
  QVERIFY(LegacyMigration::hasLegacyWork(dbPath, &h, &t));
  QCOMPARE(h, dbPath);
  // Full retry heals: fresh open recreates the schema, everything imports once.
  store->close();
  QString stashed2;
  QVERIFY(LegacyMigration::stashLegacyAtTarget(dbPath, &stashed2));
  QVERIFY(!stashed2.isEmpty());
  QVERIFY(store->reopen(dbPath));
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, stashed2, lstPath));
  QVERIFY(LegacyMigration::settleStash(dbPath, stashed2, true));
  QCOMPARE(store->listSessions().count(), 1);
  QCOMPARE(store->messagesForSession(store->listSessions().first().id).count(), 2);
  QCOMPARE(store->loadTransfers().count(), 1);
  QCOMPARE(store->loadTransfers().first().id, QStringLiteral("tid-1"));
  QVERIFY(QFile::exists(dbPath + QStringLiteral(".bak")));
}

void TstLegacyMigration::orphanStashPreserved() {
  HistoryStore* store = HistoryStore::instance();
  store->close();
  qint64 t1 = 1761300000000;
  const QString stash = dbPath + QStringLiteral(".import-tmp");
  QVERIFY(writeLegacyFile(stash, QStringLiteral("Orphan"),
                          QDateTime::fromMSecsSinceEpoch(t1), kLegacyHtml, false));
  QVERIFY(writeLegacyFile(dbPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          false));
  QString stashed;
  QVERIFY(LegacyMigration::stashLegacyAtTarget(dbPath, &stashed));
  QCOMPARE(stashed, stash);
  // Orphan bytes survive under a unique backup, never deleted.
  QDir dir(QFileInfo(dbPath).absolutePath());
  const QStringList backups =
    dir.entryList({QStringLiteral("*.import-tmp.*")}, QDir::Files);
  QCOMPARE(backups.count(), 1);
  QVERIFY(History::hasDbMarker(dir.absoluteFilePath(backups.first())));
}

void TstLegacyMigration::restoreStashReportsFailure() {
  const QString target = root + QStringLiteral("/restore-target.db");
  QFile::remove(target);
  {
    QFile f(target);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("x");
  }
  // Nothing to put back: best effort reported, not silent. The target is
  // preserved: deleting it would lose both sides with nothing to restore.
  QVERIFY(!LegacyMigration::restoreStashedLegacy(
    target, root + QStringLiteral("/no-such.import-tmp")));
  QVERIFY(QFile::exists(target));
  QFile::remove(target);
}

void TstLegacyMigration::bakCollisionUsesUniqueName() {
  HistoryStore* store = HistoryStore::instance();
  const QString legacyPath = root + QStringLiteral("/collide.db");
  const QString legacyBak = legacyPath + QStringLiteral(".bak");
  qint64 t1 = 1761300000000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          false));
  // Pre-existing .bak must not block the archive rename into an endless
  // "skipped, database not empty" loop.
  {
    QFile f(legacyBak);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("old");
  }
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, legacyPath, QString()));
  QCOMPARE(store->listSessions().count(), 1);
  QVERIFY(!QFile::exists(legacyPath));
  QVERIFY(QFile::exists(legacyBak));
  const QDir dir(QFileInfo(legacyPath).absolutePath());
  const QStringList stamped =
    dir.entryList({QFileInfo(legacyBak).fileName() + QStringLiteral(".*")}, QDir::Files);
  QCOMPARE(stamped.count(), 1);
  QFile::remove(legacyBak);
  QFile::remove(dir.absoluteFilePath(stamped.first()));
}

void TstLegacyMigration::failedImportClosesStoreForRetry() {
  // Regression for openHistory(): a failed import must not leave the store
  // open on the deleted fresh DB (writes would go to a dead inode).
  HistoryStore* store = HistoryStore::instance();
  store->close();
  qint64 t1 = 1761300000000;
  QVERIFY(writeLegacyFile(dbPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          false));
  QString stashed;
  QVERIFY(LegacyMigration::stashLegacyAtTarget(dbPath, &stashed));
  QVERIFY(!stashed.isEmpty());
  QVERIFY(store->open(dbPath));
  // Sabotage the sessions phase so the import fails.
  {
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("lmc_history"));
    QSqlQuery q(db);
    QVERIFY(q.exec(QStringLiteral("DROP TABLE sessions")));
  }
  QVERIFY(!LegacyMigration::migrateLegacyHistory(*store, stashed, QString()));
  // Fixed openHistory sequence: close before restore, history unavailable.
  store->close();
  QVERIFY(!store->isOpen());
  QVERIFY(LegacyMigration::settleStash(dbPath, stashed, false));
  QVERIFY(History::hasDbMarker(dbPath));
  QString h;
  QString t;
  QVERIFY(LegacyMigration::hasLegacyWork(dbPath, &h, &t));
  QCOMPARE(h, dbPath);
  // Full retry heals.
  store->close();
  QString stashed2;
  QVERIFY(LegacyMigration::stashLegacyAtTarget(dbPath, &stashed2));
  QVERIFY(store->reopen(dbPath));
  QVERIFY(LegacyMigration::migrateLegacyHistory(*store, stashed2, QString()));
  QVERIFY(LegacyMigration::settleStash(dbPath, stashed2, true));
  QCOMPARE(store->listSessions().count(), 1);
  QVERIFY(QFile::exists(dbPath + QStringLiteral(".bak")));
}

void TstLegacyMigration::probeQuietWithoutSources() {
  // Isolate from machine state: the history candidate is temp-controlled.
  SettingsGuard guard;
  guard.settings.setValue(IDS_SYSHISTORYPATH, false);
  guard.settings.setValue(IDS_HISTORYPATH, root + QStringLiteral("/no-such-dir/messenger.db"));
  // The transfers candidate has no override: skip when the machine is polluted.
  if(QFile::exists(StdLocation::transferHistory()))
    QSKIP("real transfers.lst present, probe would not be quiet");
  QString h;
  QString t;
  QVERIFY(!LegacyMigration::hasLegacyWork(root + QStringLiteral("/no-such.db"), &h, &t));
  QVERIFY(h.isEmpty());
  QVERIFY(t.isEmpty());
}

void TstLegacyMigration::probeFindsLegacyAtTarget() {
  QString legacyPath = root + QStringLiteral("/probe.db");
  qint64 t1 = 1761300000000;
  QVERIFY(writeLegacyFile(legacyPath, QStringLiteral("Jack"),
                          QDateTime::fromMSecsSinceEpoch(t1), structuredBlob(t1, t1 + 60000),
                          false));
  QString h;
  QString t;
  QVERIFY(LegacyMigration::hasLegacyWork(legacyPath, &h, &t));
  QCOMPARE(h, legacyPath);
}

QTEST_MAIN(TstLegacyMigration)
#include "tst_legacymigration.moc"
