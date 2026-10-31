// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "storage/historystore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QThread>

#include "core/definitions.h"
#include "core/lmccategories.h"
#include "core/settings.h"
#include "core/stdlocation.h"
#include "gui/files/filemodelview.h"
#include "legacy/history.h"
#include "legacy/historyparser.h"

#define HS_FILENAME "lanmessenger.db"
#define HS_CONNECTION "lmc_history"

namespace {

QString defaultDatabasePath() {
  return QDir::toNativeSeparators(
    QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" HS_FILENAME);
}

} // namespace

static HistoryStore* g_instance = nullptr;
static QString g_testPath;
static bool g_hasTestPath = false;

HistoryStore::HistoryStore(QObject* parent)
    : QObject(parent), m_connectionName(HS_CONNECTION) {
}

HistoryStore* HistoryStore::instance() {
  if(!g_instance)
    g_instance = new HistoryStore();
  return g_instance;
}

void HistoryStore::setDatabasePathForTests(const QString& path) {
  g_testPath = path;
  g_hasTestPath = true;
}

void HistoryStore::resetForTests() {
  if(QSqlDatabase::contains(HS_CONNECTION)) {
    {
      QSqlDatabase db = QSqlDatabase::database(HS_CONNECTION);
      db.close();
    }
    QSqlDatabase::removeDatabase(HS_CONNECTION);
  }
  delete g_instance;
  g_instance = nullptr;
  g_testPath.clear();
  g_hasTestPath = false;
}

QString HistoryStore::uniqueBackupPath(const QString& base) {
  if(!QFile::exists(base))
    return base;
  const QString stamp =
    QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"));
  QString candidate = base + QStringLiteral(".") + stamp;
  if(!QFile::exists(candidate))
    return candidate;
  for(int n = 1;; ++n) {
    candidate =
      base + QStringLiteral(".") + stamp + QStringLiteral("-") + QString::number(n);
    if(!QFile::exists(candidate))
      return candidate;
  }
}

QString HistoryStore::databaseFile() {
  if(g_hasTestPath)
    return g_testPath;
  lmcSettings settings;
  bool sysPath = settings.value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool();
  if(sysPath)
    return defaultDatabasePath();
  return settings.value(IDS_HISTORYPATH, defaultDatabasePath()).toString();
}

void HistoryStore::checkAffinity() const {
  Q_ASSERT(thread() == QThread::currentThread());
}

bool HistoryStore::open(const QString& path) {
  checkAffinity();
  const QString prevPath = m_path;
  m_path = path.isEmpty() ? databaseFile() : path;

  // A legacy messenger.db at the target path is not SQLite: stash it aside,
  // create a fresh DB, then import from the stash.
  QString legacyAtTarget;
  if(m_path != QStringLiteral(":memory:") && !g_hasTestPath &&
     QFile::exists(m_path) && History::hasDbMarker(m_path)) {
    legacyAtTarget = m_path + QStringLiteral(".import-tmp");
    QFile::remove(legacyAtTarget);
    if(!QFile::rename(m_path, legacyAtTarget)) {
      qCWarning(lmcApp(), "HistoryStore: cannot stash legacy %s", qPrintable(m_path));
      m_path = prevPath;
      return false;
    }
  }

  if(m_path != QStringLiteral(":memory:")) {
    QDir dir = QFileInfo(m_path).dir();
    if(!dir.exists() && !dir.mkpath(dir.absolutePath())) {
      qCWarning(lmcApp(), "HistoryStore: cannot create dir %s",
                qPrintable(dir.absolutePath()));
      // Restore path and stashed legacy: the old connection may still be open.
      if(!legacyAtTarget.isEmpty()) {
        QFile::remove(m_path);
        QFile::rename(legacyAtTarget, m_path);
      }
      m_path = prevPath;
      return false;
    }
  }

  if(QSqlDatabase::contains(m_connectionName)) {
    {
      QSqlDatabase old = QSqlDatabase::database(m_connectionName);
      old.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
  }
  QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
  db.setDatabaseName(m_path);
  if(!db.open()) {
    qCWarning(lmcApp(), "HistoryStore: open failed %s: %s", qPrintable(m_path),
              qPrintable(db.lastError().text()));
    if(!legacyAtTarget.isEmpty()) {
      QFile::remove(m_path);
      QFile::rename(legacyAtTarget, m_path);
    }
    // Connection already removed: mark closed so reopenIfPathChanged() retries.
    m_open = false;
    return false;
  }

  QSqlQuery pragma(db);
  pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
  pragma.exec(QStringLiteral("PRAGMA busy_timeout=5000"));
  pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
  m_open = true;

  if(!ensureSchema()) {
    close();
    if(!legacyAtTarget.isEmpty()) {
      QFile::remove(m_path);
      QFile::rename(legacyAtTarget, m_path);
    }
    return false;
  }
  maybeImportLegacy(legacyAtTarget);
  // Move stash leftovers next to the DB as <db>.bak (never overwrite).
  if(!legacyAtTarget.isEmpty()) {
    QString wrongBak = legacyAtTarget + QStringLiteral(".bak");
    QString rightBak = m_path + QStringLiteral(".bak");
    QString toPreserve;
    if(QFile::exists(wrongBak))
      toPreserve = wrongBak;
    else if(QFile::exists(legacyAtTarget))
      toPreserve = legacyAtTarget;
    if(!toPreserve.isEmpty()) {
      const QString dest = uniqueBackupPath(rightBak);
      if(!QFile::rename(toPreserve, dest))
        qCWarning(lmcApp(), "HistoryStore: cannot preserve %s as %s",
                  qPrintable(toPreserve), qPrintable(dest));
    }
  }
  return true;
}

bool HistoryStore::isOpen() const {
  return m_open && QSqlDatabase::contains(m_connectionName);
}

void HistoryStore::close() {
  checkAffinity();
  m_open = false;
  if(!QSqlDatabase::contains(m_connectionName))
    return;
  {
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    db.close();
  }
  QSqlDatabase::removeDatabase(m_connectionName);
}

bool HistoryStore::reopenIfPathChanged() {
  checkAffinity();
  if(g_hasTestPath)
    return true;
  QString wanted = databaseFile();
  if(m_open && m_path == wanted)
    return true;
  if(!m_open && m_path == wanted) {
    // Retry the failed open; on success the views resync via the signals below.
    bool ok = open(wanted);
    if(ok) {
      emit sessionsChanged();
      emit transfersChanged();
    }
    return ok;
  }
  close();
  bool ok = open(wanted);
  if(ok) {
    emit sessionsChanged();
    emit transfersChanged();
  }
  return ok;
}

bool HistoryStore::ensureSchema() {
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  QSqlQuery q(db);
  if(!q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS sessions("
                            "id INTEGER PRIMARY KEY, "
                            "name TEXT NOT NULL, "
                            "closed_at INTEGER NOT NULL)"))) {
    qCWarning(lmcApp(), "HistoryStore: sessions DDL failed: %s",
              qPrintable(q.lastError().text()));
    return false;
  }
  if(!q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS messages("
                            "id INTEGER PRIMARY KEY, "
                            "session_id INTEGER NOT NULL REFERENCES sessions(id), "
                            "sender_id TEXT NOT NULL, "
                            "sender_name TEXT NOT NULL, "
                            "is_own INTEGER NOT NULL, "
                            "message_type INTEGER NOT NULL, "
                            "created_at INTEGER NOT NULL, "
                            "body TEXT NOT NULL)"))) {
    qCWarning(lmcApp(), "HistoryStore: messages DDL failed: %s",
              qPrintable(q.lastError().text()));
    return false;
  }
  if(!q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_messages_session_created "
                            "ON messages(session_id, created_at)"))) {
    qCWarning(lmcApp(), "HistoryStore: messages index failed: %s",
              qPrintable(q.lastError().text()));
    return false;
  }
  // Composite PK (id, mode): one id may appear in both TM_Send and TM_Receive rows.
  if(!q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS transfers("
                            "id TEXT NOT NULL, "
                            "mode INTEGER NOT NULL, "
                            "message_type INTEGER NOT NULL, "
                            "file_path TEXT, "
                            "file_display TEXT, "
                            "user_id TEXT, "
                            "user_name TEXT, "
                            "file_size INTEGER, "
                            "state INTEGER NOT NULL, "
                            "created_at INTEGER NOT NULL, "
                            "PRIMARY KEY (id, mode))"))) {
    qCWarning(lmcApp(), "HistoryStore: transfers DDL failed: %s",
              qPrintable(q.lastError().text()));
    return false;
  }
  return true;
}

bool HistoryStore::sessionsEmpty() const {
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  QSqlQuery q(db);
  if(!q.exec(QStringLiteral("SELECT COUNT(*) FROM sessions")))
    return true;
  if(q.next())
    return q.value(0).toLongLong() == 0;
  return true;
}

bool HistoryStore::transfersEmpty() const {
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  QSqlQuery q(db);
  if(!q.exec(QStringLiteral("SELECT COUNT(*) FROM transfers")))
    return true;
  if(q.next())
    return q.value(0).toLongLong() == 0;
  return true;
}

bool HistoryStore::saveSession(const QString& name, const QDateTime& closedAt,
                               const QList<StoredMessage>& messages) {
  checkAffinity();
  if(!isOpen())
    return false;
  // Always write the session row, even with no text messages (legacy parity).
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  if(!db.transaction()) {
    qCWarning(lmcApp(), "HistoryStore: session transaction start failed: %s",
              qPrintable(db.lastError().text()));
    return false;
  }
  QSqlQuery q(db);
  q.prepare(QStringLiteral("INSERT INTO sessions(name, closed_at) VALUES(?, ?)"));
  q.addBindValue(name);
  q.addBindValue(closedAt.toMSecsSinceEpoch());
  if(!q.exec()) {
    qCWarning(lmcApp(), "HistoryStore: insert session failed: %s",
              qPrintable(q.lastError().text()));
    db.rollback();
    return false;
  }
  qint64 sessionId = q.lastInsertId().toLongLong();
  QSqlQuery m(db);
  m.prepare(QStringLiteral("INSERT INTO messages(session_id, sender_id, sender_name, "
                           "is_own, message_type, created_at, body) "
                           "VALUES(?, ?, ?, ?, ?, ?, ?)"));
  for(const StoredMessage& msg : messages) {
    m.addBindValue(sessionId);
    m.addBindValue(msg.senderId);
    m.addBindValue(msg.senderName);
    m.addBindValue(msg.isOwn ? 1 : 0);
    m.addBindValue(msg.messageType);
    m.addBindValue(msg.createdAt);
    m.addBindValue(msg.body);
    if(!m.exec()) {
      qCWarning(lmcApp(), "HistoryStore: insert message failed: %s",
                qPrintable(m.lastError().text()));
      db.rollback();
      return false;
    }
  }
  if(!db.commit()) {
    qCWarning(lmcApp(), "HistoryStore: session commit failed: %s",
              qPrintable(db.lastError().text()));
    db.rollback();
    return false;
  }
  emit sessionsChanged();
  return true;
}

QList<SessionInfo> HistoryStore::listSessions() const {
  checkAffinity();
  QList<SessionInfo> out;
  if(!isOpen())
    return out;
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  QSqlQuery q(db);
  if(!q.exec(QStringLiteral("SELECT id, name, closed_at FROM sessions "
                            "ORDER BY closed_at DESC, id DESC"))) {
    qCWarning(lmcApp(), "HistoryStore: listSessions failed: %s",
              qPrintable(q.lastError().text()));
    return out;
  }
  while(q.next()) {
    SessionInfo s;
    s.id = q.value(0).toLongLong();
    s.name = q.value(1).toString();
    s.closedAt = QDateTime::fromMSecsSinceEpoch(q.value(2).toLongLong());
    out.append(s);
  }
  return out;
}

QList<StoredMessage> HistoryStore::messagesForSession(qint64 sessionId) const {
  checkAffinity();
  QList<StoredMessage> out;
  if(!isOpen())
    return out;
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  QSqlQuery q(db);
  q.prepare(QStringLiteral("SELECT sender_id, sender_name, is_own, message_type, "
                           "created_at, body FROM messages "
                           "WHERE session_id=? ORDER BY created_at ASC, id ASC"));
  q.addBindValue(sessionId);
  if(!q.exec()) {
    qCWarning(lmcApp(), "HistoryStore: messagesForSession failed: %s",
              qPrintable(q.lastError().text()));
    return out;
  }
  while(q.next()) {
    StoredMessage m;
    m.senderId = q.value(0).toString();
    m.senderName = q.value(1).toString();
    m.isOwn = q.value(2).toInt() != 0;
    m.messageType = q.value(3).toInt();
    m.createdAt = q.value(4).toLongLong();
    m.body = q.value(5).toString();
    out.append(m);
  }
  return out;
}

bool HistoryStore::clearSessions() {
  checkAffinity();
  if(!isOpen())
    return false;
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  if(!db.transaction()) {
    qCWarning(lmcApp(), "HistoryStore: clearSessions cannot start transaction: %s",
              qPrintable(db.lastError().text()));
    return false;
  }
  QSqlQuery q(db);
  if(!q.exec(QStringLiteral("DELETE FROM messages")) ||
     !q.exec(QStringLiteral("DELETE FROM sessions"))) {
    qCWarning(lmcApp(), "HistoryStore: clearSessions failed: %s",
              qPrintable(q.lastError().text()));
    db.rollback();
    return false;
  }
  if(!db.commit()) {
    qCWarning(lmcApp(), "HistoryStore: clearSessions commit failed: %s",
              qPrintable(db.lastError().text()));
    db.rollback();
    return false;
  }
  emit sessionsChanged();
  return true;
}

bool HistoryStore::saveTransfers(const QList<TransferRecord>& records) {
  checkAffinity();
  if(!isOpen())
    return false;
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  if(!db.transaction()) {
    qCWarning(lmcApp(), "HistoryStore: transfers transaction start failed: %s",
              qPrintable(db.lastError().text()));
    return false;
  }
  QSqlQuery del(db);
  if(!del.exec(QStringLiteral("DELETE FROM transfers"))) {
    qCWarning(lmcApp(), "HistoryStore: clear transfers failed: %s",
              qPrintable(del.lastError().text()));
    db.rollback();
    return false;
  }
  QSqlQuery q(db);
  q.prepare(QStringLiteral("INSERT OR REPLACE INTO transfers(id, mode, message_type, "
                           "file_path, file_display, user_id, user_name, file_size, "
                           "state, created_at) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
  for(const TransferRecord& r : records) {
    q.addBindValue(r.id);
    q.addBindValue(r.mode);
    q.addBindValue(r.messageType != 0 ? r.messageType : int(MT_File));
    q.addBindValue(r.filePath);
    q.addBindValue(r.fileDisplay);
    q.addBindValue(r.userId);
    q.addBindValue(r.userName);
    q.addBindValue(r.fileSize);
    q.addBindValue(r.state);
    q.addBindValue(r.createdAt);
    if(!q.exec()) {
      qCWarning(lmcApp(), "HistoryStore: insert transfer failed: %s",
                qPrintable(q.lastError().text()));
      db.rollback();
      return false;
    }
  }
  if(!db.commit()) {
    qCWarning(lmcApp(), "HistoryStore: transfers commit failed: %s",
              qPrintable(db.lastError().text()));
    db.rollback();
    return false;
  }
  emit transfersChanged();
  return true;
}

QList<TransferRecord> HistoryStore::loadTransfers() const {
  checkAffinity();
  QList<TransferRecord> out;
  if(!isOpen())
    return out;
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  QSqlQuery q(db);
  if(!q.exec(QStringLiteral("SELECT id, mode, message_type, file_path, file_display, "
                            "user_id, user_name, file_size, state, created_at "
                            "FROM transfers ORDER BY created_at ASC, rowid ASC"))) {
    qCWarning(lmcApp(), "HistoryStore: loadTransfers failed: %s",
              qPrintable(q.lastError().text()));
    return out;
  }
  while(q.next()) {
    TransferRecord r;
    r.id = q.value(0).toString();
    r.mode = q.value(1).toInt();
    r.messageType = q.value(2).toInt();
    if(r.messageType != int(MT_File) && r.messageType != int(MT_Folder))
      r.messageType = int(MT_File);
    if(r.mode != int(FileView::TM_Send) && r.mode != int(FileView::TM_Receive))
      r.mode = int(FileView::TM_Send);
    r.filePath = q.value(3).toString();
    r.fileDisplay = q.value(4).toString();
    r.userId = q.value(5).toString();
    r.userName = q.value(6).toString();
    r.fileSize = q.value(7).toLongLong();
    r.state = q.value(8).toInt();
    if(r.state < 0 || r.state >= int(FileView::TS_Max))
      r.state = int(FileView::TS_Complete);
    r.createdAt = q.value(9).toLongLong();
    out.append(r);
  }
  return out;
}

bool HistoryStore::clearTransfers() {
  checkAffinity();
  if(!isOpen())
    return false;
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  QSqlQuery q(db);
  if(!q.exec(QStringLiteral("DELETE FROM transfers"))) {
    qCWarning(lmcApp(), "HistoryStore: clearTransfers failed: %s",
              qPrintable(q.lastError().text()));
    return false;
  }
  emit transfersChanged();
  return true;
}

bool HistoryStore::removeTransfer(const QString& id, int mode) {
  checkAffinity();
  if(!isOpen())
    return false;
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  QSqlQuery q(db);
  q.prepare(QStringLiteral("DELETE FROM transfers WHERE id=? AND mode=?"));
  q.addBindValue(id);
  q.addBindValue(mode);
  if(!q.exec()) {
    qCWarning(lmcApp(), "HistoryStore: removeTransfer failed: %s",
              qPrintable(q.lastError().text()));
    return false;
  }
  // "True" means SQL ok, not "a row existed": emit only when a stored row
  // actually disappeared, so a no-op delete does not reload the view.
  if(q.numRowsAffected() > 0)
    emit transfersChanged();
  return true;
}

void HistoryStore::maybeImportLegacy(const QString& stashedLegacy) {
  // One-shot idempotent import; sessions and transfers gate independently.
  if(g_hasTestPath)
    return;
  lmcSettings settings;
  bool sysPath =
    settings.value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool();
  QString candidate;
  bool hasCandidate = false;
  if(!stashedLegacy.isEmpty()) {
    candidate = stashedLegacy;
    hasCandidate = History::hasDbMarker(candidate);
  } else if(!sysPath) {
    // Empty custom path falls back to the default messenger.db.
    candidate = History::historyFile();
    if(candidate.isEmpty())
      candidate = QDir::toNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
        QStringLiteral("/messenger.db"));
    hasCandidate = !candidate.isEmpty() && History::hasDbMarker(candidate);
    // Never read the just-created SQLite file as legacy.
    if(hasCandidate && !m_path.isEmpty() &&
       QFileInfo(candidate).absoluteFilePath() == QFileInfo(m_path).absoluteFilePath())
      hasCandidate = false;
  } else {
    candidate = QDir::toNativeSeparators(
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
      QStringLiteral("/messenger.db"));
    hasCandidate = History::hasDbMarker(candidate);
  }
  QString transferPath = StdLocation::transferHistory();
  bool hasTransferLegacy = QFile::exists(transferPath);
  bool needSessions = hasCandidate && sessionsEmpty();
  bool needTransfers = hasTransferLegacy && transfersEmpty();
  if(!needSessions && !needTransfers) {
    if(hasCandidate && !sessionsEmpty())
      qCWarning(lmcApp(), "HistoryStore: legacy %s skipped, database not empty",
                qPrintable(candidate));
    return;
  }
  importLegacyFrom(needSessions ? candidate : QString(),
                   needTransfers ? transferPath : QString());
}

bool HistoryStore::importLegacyFrom(const QString& historyPath,
                                    const QString& transferPath) {
  checkAffinity();
  if(!isOpen())
    return false;
  bool hasCandidate = !historyPath.isEmpty() && QFile::exists(historyPath);
  bool hasTransferLegacy = !transferPath.isEmpty() && QFile::exists(transferPath);
  if(!hasCandidate && !hasTransferLegacy)
    return true;
  // Never import the open SQLite file itself.
  if(hasCandidate && m_path != QStringLiteral(":memory:") &&
     QFileInfo(historyPath).absoluteFilePath() == QFileInfo(m_path).absoluteFilePath()) {
    qCWarning(lmcApp(), "HistoryStore: legacy %s is the open database, skipped",
              qPrintable(historyPath));
    hasCandidate = false;
  }
  bool needSessions = hasCandidate && sessionsEmpty();
  bool needTransfers = hasTransferLegacy && transfersEmpty();
  if(!needSessions && !needTransfers) {
    if(hasCandidate && !sessionsEmpty())
      qCWarning(lmcApp(), "HistoryStore: legacy %s skipped, database not empty",
                qPrintable(historyPath));
    return true;
  }

  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  if(!db.transaction()) {
    qCWarning(lmcApp(), "HistoryStore: legacy import cannot start transaction");
    return false;
  }

  int importedSessions = 0;
  int importedMessages = 0;
  int skippedBlobs = 0;
  int importedTransfers = 0;
  int skippedTransfers = 0;
  bool ok = true;

  if(needSessions) {
    QList<MsgInfo> legacy = History::getListFrom(historyPath);
    QSqlQuery sIns(db);
    sIns.prepare(QStringLiteral("INSERT INTO sessions(name, closed_at) VALUES(?, ?)"));
    QSqlQuery mIns(db);
    mIns.prepare(QStringLiteral("INSERT INTO messages(session_id, sender_id, sender_name, "
                                "is_own, message_type, created_at, body) "
                                "VALUES(?, ?, ?, ?, ?, ?, ?)"));
    for(const MsgInfo& info : legacy) {
      QString blob = History::getMessageFrom(historyPath, info.offset);
      QList<HistoryRecord> records;
      QList<StoredMessage> rows;
      if(parseStructuredHistory(blob, &records)) {
        for(const HistoryRecord& rec : records) {
          StoredMessage m;
          m.senderId = rec.userName;
          m.senderName = rec.userName;
          m.isOwn = rec.own;
          m.messageType = rec.type;
          m.createdAt = rec.ts > 0 ? rec.ts : info.date.toMSecsSinceEpoch();
          m.body = rec.body;
          rows.append(m);
        }
      } else {
        QString plain = legacyHtmlToPlain(blob);
        if(plain.isEmpty()) {
          skippedBlobs++;
          continue;
        }
        StoredMessage m;
        m.senderId = info.name;
        m.senderName = info.name;
        m.isOwn = false;
        m.messageType = int(MT_Message);
        m.createdAt = info.date.toMSecsSinceEpoch();
        m.body = plain;
        rows.append(m);
      }
      if(rows.isEmpty()) {
        skippedBlobs++;
        continue;
      }
      sIns.addBindValue(info.name);
      sIns.addBindValue(info.date.toMSecsSinceEpoch());
      if(!sIns.exec()) {
        qCWarning(lmcApp(), "HistoryStore: legacy session insert failed: %s",
                  qPrintable(sIns.lastError().text()));
        ok = false;
        break;
      }
      qint64 sessionId = sIns.lastInsertId().toLongLong();
      for(const StoredMessage& m : rows) {
        mIns.addBindValue(sessionId);
        mIns.addBindValue(m.senderId);
        mIns.addBindValue(m.senderName);
        mIns.addBindValue(m.isOwn ? 1 : 0);
        mIns.addBindValue(m.messageType);
        mIns.addBindValue(m.createdAt);
        mIns.addBindValue(m.body);
        if(!mIns.exec()) {
          qCWarning(lmcApp(), "HistoryStore: legacy message insert failed: %s",
                    qPrintable(mIns.lastError().text()));
          ok = false;
          break;
        }
        importedMessages++;
      }
      if(!ok)
        break;
      importedSessions++;
    }
  }

  if(ok && needTransfers) {
    FileModel model;
    model.loadData(transferPath);
    QSqlQuery tIns(db);
    tIns.prepare(QStringLiteral("INSERT OR IGNORE INTO transfers(id, mode, message_type, "
                                "file_path, file_display, user_id, user_name, "
                                "file_size, state, created_at) "
                                "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    for(int i = 0; i < model.rowCount(); ++i) {
      FileView* v = model.item(i);
      if(!v || v->id.isEmpty()) {
        skippedTransfers++;
        continue;
      }
      // Clamp: never trust an enum from a legacy byte stream.
      const int mode =
        (v->mode == FileView::TM_Send || v->mode == FileView::TM_Receive)
          ? int(v->mode)
          : int(FileView::TM_Send);
      const int state =
        (v->state >= 0 && v->state < int(FileView::TS_Max)) ? int(v->state)
                                                            : int(FileView::TS_Complete);
      tIns.addBindValue(v->id);
      tIns.addBindValue(mode);
      // Legacy rows never stored a type: always files.
      tIns.addBindValue(int(MT_File));
      tIns.addBindValue(v->filePath);
      tIns.addBindValue(v->fileDisplay);
      // Legacy stream has no userId/fileSize: fall back to userName/0.
      tIns.addBindValue(v->userId.isEmpty() ? v->userName : v->userId);
      tIns.addBindValue(v->userName);
      tIns.addBindValue(v->fileSize);
      tIns.addBindValue(state);
      tIns.addBindValue(QDateTime::currentMSecsSinceEpoch());
      if(!tIns.exec()) {
        qCWarning(lmcApp(), "HistoryStore: legacy transfer insert failed: %s",
                  qPrintable(tIns.lastError().text()));
        skippedTransfers++;
        continue;
      }
      importedTransfers++;
    }
  }

  if(!ok) {
    db.rollback();
    return false;
  }
  if(!db.commit()) {
    qCWarning(lmcApp(), "HistoryStore: legacy import commit failed");
    db.rollback();
    return false;
  }

  // Rename sources to .bak even when empty, or they rescan every launch.
  if(needSessions &&
     !QFile::rename(historyPath, historyPath + QStringLiteral(".bak")))
    qCWarning(lmcApp(), "HistoryStore: cannot rename %s to .bak", qPrintable(historyPath));
  if(needTransfers &&
     !QFile::rename(transferPath, transferPath + QStringLiteral(".bak")))
    qCWarning(lmcApp(), "HistoryStore: cannot rename %s to .bak", qPrintable(transferPath));
  qCInfo(lmcApp(),
         "HistoryStore: legacy import sessions=%d messages=%d skippedBlobs=%d "
         "transfers=%d skippedTransfers=%d",
         importedSessions, importedMessages, skippedBlobs, importedTransfers,
         skippedTransfers);
  if(importedSessions > 0)
    emit sessionsChanged();
  if(importedTransfers > 0)
    emit transfersChanged();
  return true;
}
