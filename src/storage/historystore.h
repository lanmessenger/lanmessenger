// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

// Plain structures cross the store boundary. No QtSql, no GUI types,
// no table names outside src/storage/ and the store tests.
struct StoredMessage {
  QString senderId;
  QString senderName;
  bool isOwn = false;
  int messageType = 0;
  qint64 createdAt = 0;
  QString body;
};

struct SessionInfo {
  qint64 id = 0;
  QString name;
  QDateTime closedAt;
};

struct TransferRecord {
  QString id;
  int mode = 0;
  int messageType = 0;
  QString filePath;
  QString fileDisplay;
  QString userId;
  QString userName;
  qint64 fileSize = 0;
  int state = 0;
  qint64 createdAt = 0;
};

class HistoryStore : public QObject {
  Q_OBJECT

public:
  static HistoryStore* instance();
  // Test seam: isolated DB path (:memory:/temp) + instance reset.
  static void setDatabasePathForTests(const QString& path);
  static void resetForTests();

  static QString databaseFile();

  bool open(const QString& path = QString());
  bool isOpen() const;
  void close();
  bool reopenIfPathChanged();

  bool saveSession(const QString& name, const QDateTime& closedAt,
                   const QList<StoredMessage>& messages);
  QList<SessionInfo> listSessions() const;
  QList<StoredMessage> messagesForSession(qint64 sessionId) const;
  bool clearSessions();

  bool saveTransfers(const QList<TransferRecord>& records);
  QList<TransferRecord> loadTransfers() const;
  bool clearTransfers();
  // Composite key (id, mode) matches transferKey() in the view: one id may
  // legally appear in both TM_Send and TM_Receive rows.
  // Returns true when the SQL delete succeeds, even if no row matched
  // (the view also drops unsaved rows locally); emits transfersChanged
  // only when a stored row actually disappeared.
  bool removeTransfer(const QString& id, int mode);

  // Test seam for the legacy importer with explicit paths (no settings I/O).
  bool importLegacyFrom(const QString& historyPath, const QString& transferPath);

  // Next free backup path for base: base itself when free, else
  // base.<yyyyMMdd-hhmmss> with a -N suffix on collision (two imports can
  // land in the same second). Used for legacy .bak preservation.
  static QString uniqueBackupPath(const QString& base);

signals:
  void sessionsChanged();
  void transfersChanged();

private:
  explicit HistoryStore(QObject* parent = nullptr);
  void checkAffinity() const;
  bool ensureSchema();
  bool sessionsEmpty() const;
  bool transfersEmpty() const;
  void maybeImportLegacy(const QString& stashedLegacy = QString());

  QString m_connectionName;
  QString m_path;
  bool m_open = false;
};
