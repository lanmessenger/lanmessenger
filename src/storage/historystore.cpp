// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "storage/historystore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

#include "core/definitions.h"
#include "core/lmccategories.h"
#include "gui/files/filemodelview.h"

#define HS_CONNECTION "lmc_history"

namespace {

constexpr int kSchemaVersion = 1;

} // namespace

static HistoryStore* g_instance = nullptr;

HistoryStore::HistoryStore(QObject* parent)
    : QObject(parent), m_connectionName(HS_CONNECTION) {
}

HistoryStore* HistoryStore::instance() {
  if(!g_instance)
    g_instance = new HistoryStore();
  return g_instance;
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

void HistoryStore::checkAffinity() const {
  Q_ASSERT(thread() == QThread::currentThread());
}

bool HistoryStore::open(const QString& path) {
  checkAffinity();
  if(path.isEmpty()) {
    qCWarning(lmcApp(), "HistoryStore: refused to open with empty path");
    return false;
  }
  const QString prevPath = m_path;
  m_path = path;
  // Invariant: a failed open restores m_path below, so bookkeeping never
  // claims a database that is not open. Callers must pass a real SQLite
  // target: a legacy binary opened here would pass the version gate (0)
  // and get our tables stamped into it. openHistory() stashes legacy
  // binaries aside via stashLegacyAtTarget() before calling open().

  if(m_path != QStringLiteral(":memory:")) {
    QDir dir = QFileInfo(m_path).dir();
    if(!dir.exists() && !dir.mkpath(dir.absolutePath())) {
      qCWarning(lmcApp(), "HistoryStore: cannot create dir %s",
                qPrintable(dir.absolutePath()));
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
  // Scope the handle/query: failure paths below call close() ->
  // removeDatabase(), which warns if any handle is still alive.
  {
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_path);
    if(!db.open()) {
      qCWarning(lmcApp(), "HistoryStore: open failed %s: %s", qPrintable(m_path),
                qPrintable(db.lastError().text()));
      // Connection already removed: mark closed so reopen() retries.
      m_open = false;
      m_path = prevPath;
      return false;
    }

    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA busy_timeout=5000"));
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
  }

  const int stored = getUserVersion();
  if(stored < 0 || stored > kSchemaVersion) {
    qCWarning(lmcApp(), "HistoryStore: unsupported schema version %d (supported %d)", stored,
              kSchemaVersion);
    close();
    m_path = prevPath;
    return false;
  }
  if(!ensureSchema(stored)) {
    close();
    m_path = prevPath;
    return false;
  }
  m_open = true;
  emit sessionsChanged();
  emit transfersChanged();
  return true;
}

bool HistoryStore::isOpen() const {
  return m_open && QSqlDatabase::contains(m_connectionName);
}

QString HistoryStore::databasePath() const {
  return m_path;
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

bool HistoryStore::reopen(const QString& wanted) {
  checkAffinity();
  if(m_open && m_path == wanted)
    return true;
  // Retry the failed open, or switch databases: on success open() notifies.
  if(m_open)
    close();
  return open(wanted);
}

bool HistoryStore::ensureSchema(int storedVersion) {
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
  // Reached only for stored 0 or kSchemaVersion (open() refused the rest):
  // create tables, stamp version, done.
  if(storedVersion != 0)
    return true;
  QSqlQuery stamp(db);
  if(!stamp.exec(QStringLiteral("PRAGMA user_version = %1").arg(kSchemaVersion))) {
    qCWarning(lmcApp(), "HistoryStore: cannot stamp schema version: %s",
              qPrintable(stamp.lastError().text()));
    return false;
  }
  return true;
}

int HistoryStore::getUserVersion() {
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  QSqlQuery q(db);
  if(!q.exec(QStringLiteral("PRAGMA user_version"))) {
    qCWarning(lmcApp(), "HistoryStore: cannot read schema version: %s",
              qPrintable(q.lastError().text()));
    return -1;
  }
  if(!q.next()) {
    qCWarning(lmcApp(), "HistoryStore: schema version query returned no rows");
    return -1;
  }
  return q.value(0).toInt();
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

bool HistoryStore::saveSessionBatch(const QList<SessionToSave>& sessions) {
  checkAffinity();
  if(!isOpen())
    return false;
  if(sessions.isEmpty())
    return true;
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);
  if(!db.transaction()) {
    qCWarning(lmcApp(), "HistoryStore: session batch transaction start failed: %s",
              qPrintable(db.lastError().text()));
    return false;
  }
  QSqlQuery sIns(db);
  sIns.prepare(QStringLiteral("INSERT INTO sessions(name, closed_at) VALUES(?, ?)"));
  QSqlQuery mIns(db);
  mIns.prepare(QStringLiteral("INSERT INTO messages(session_id, sender_id, sender_name, "
                              "is_own, message_type, created_at, body) "
                              "VALUES(?, ?, ?, ?, ?, ?, ?)"));
  for(const SessionToSave& session : sessions) {
    sIns.addBindValue(session.name);
    sIns.addBindValue(session.closedAt.toMSecsSinceEpoch());
    if(!sIns.exec()) {
      qCWarning(lmcApp(), "HistoryStore: batch session insert failed: %s",
                qPrintable(sIns.lastError().text()));
      db.rollback();
      return false;
    }
    qint64 sessionId = sIns.lastInsertId().toLongLong();
    for(const StoredMessage& msg : session.messages) {
      mIns.addBindValue(sessionId);
      mIns.addBindValue(msg.senderId);
      mIns.addBindValue(msg.senderName);
      mIns.addBindValue(msg.isOwn ? 1 : 0);
      mIns.addBindValue(msg.messageType);
      mIns.addBindValue(msg.createdAt);
      mIns.addBindValue(msg.body);
      if(!mIns.exec()) {
        qCWarning(lmcApp(), "HistoryStore: batch message insert failed: %s",
                  qPrintable(mIns.lastError().text()));
        db.rollback();
        return false;
      }
    }
  }
  if(!db.commit()) {
    qCWarning(lmcApp(), "HistoryStore: session batch commit failed: %s",
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
