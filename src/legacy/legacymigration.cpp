// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "legacy/legacymigration.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

#include "core/definitions.h"
#include "core/lmccategories.h"
#include "core/settings.h"
#include "core/stdlocation.h"
#include "gui/files/filemodelview.h"
#include "legacy/history.h"
#include "legacy/historyparser.h"
#include "storage/historystore.h"

bool LegacyMigration::hasLegacyWork(const QString& dbTarget, QString* historyPath,
                                    QString* transferPath) {
  legacyCandidates(historyPath, transferPath);
  const bool atTarget = dbTarget != QStringLiteral(":memory:") &&
                        QFile::exists(dbTarget) && History::hasDbMarker(dbTarget);
  if(atTarget && historyPath)
    *historyPath = dbTarget;
  return atTarget || (historyPath && !historyPath->isEmpty()) ||
         (transferPath && !transferPath->isEmpty());
}

bool LegacyMigration::stashLegacyAtTarget(const QString& targetPath,
                                          QString* stashedLegacy) {
  if(stashedLegacy)
    stashedLegacy->clear();
  if(targetPath == QStringLiteral(":memory:") || !QFile::exists(targetPath) ||
     !History::hasDbMarker(targetPath))
    return true;
  const QString stash = targetPath + QStringLiteral(".import-tmp");
  if(QFile::exists(stash)) {
    // A previous run crashed between stash and settle: never delete the
    // orphan, move it aside (hasLegacyWork does not probe it).
    const QString dest = HistoryStore::uniqueBackupPath(stash);
    if(!QFile::rename(stash, dest)) {
      qCWarning(lmcApp(), "LegacyMigration: cannot preserve orphan stash %s",
                qPrintable(stash));
      return false;
    }
  }
  if(!QFile::rename(targetPath, stash)) {
    qCWarning(lmcApp(), "LegacyMigration: cannot stash legacy %s",
              qPrintable(targetPath));
    return false;
  }
  if(stashedLegacy)
    *stashedLegacy = stash;
  return true;
}

bool LegacyMigration::restoreStashedLegacy(const QString& targetPath,
                                           const QString& stashedLegacy) {
  if(stashedLegacy.isEmpty())
    return true;
  if(!QFile::exists(stashedLegacy)) {
    qCWarning(lmcApp(), "LegacyMigration: no stash %s, target %s preserved",
              qPrintable(stashedLegacy), qPrintable(targetPath));
    return false;
  }
  if(QFile::exists(targetPath) && !QFile::remove(targetPath)) {
    qCWarning(lmcApp(), "LegacyMigration: cannot remove %s to restore stash",
              qPrintable(targetPath));
    return false;
  }
  if(!QFile::rename(stashedLegacy, targetPath)) {
    qCWarning(lmcApp(), "LegacyMigration: cannot restore stash %s to %s",
              qPrintable(stashedLegacy), qPrintable(targetPath));
    return false;
  }
  return true;
}

void LegacyMigration::legacyCandidates(QString* historyPath, QString* transferPath) {
  if(historyPath) {
    historyPath->clear();
    lmcSettings settings;
    const bool sysPath =
      settings.value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool();
    QString candidate;
    if(!sysPath) {
      candidate = History::historyFile();
      if(candidate.isEmpty())
        candidate = QDir::toNativeSeparators(
          QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
          QStringLiteral("/messenger.db"));
    } else {
      candidate = QDir::toNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
        QStringLiteral("/messenger.db"));
    }
    if(!candidate.isEmpty() && History::hasDbMarker(candidate))
      *historyPath = candidate;
  }
  if(transferPath) {
    transferPath->clear();
    const QString candidate = StdLocation::transferHistory();
    if(QFile::exists(candidate))
      *transferPath = candidate;
  }
}

bool LegacyMigration::migrateLegacyHistory(HistoryStore& store,
                                           const QString& historyPath,
                                           const QString& transferPath) {
  if(!store.isOpen())
    return false;
  bool hasCandidate = !historyPath.isEmpty() && QFile::exists(historyPath);
  bool hasTransferLegacy = !transferPath.isEmpty() && QFile::exists(transferPath);
  if(!hasCandidate && !hasTransferLegacy)
    return true;
  if(hasCandidate && store.databasePath() != QStringLiteral(":memory:") &&
     QFileInfo(historyPath).absoluteFilePath() ==
       QFileInfo(store.databasePath()).absoluteFilePath()) {
    qCWarning(lmcApp(), "LegacyMigration: legacy %s is the open database, skipped",
              qPrintable(historyPath));
    hasCandidate = false;
  }
  const bool needSessions = hasCandidate && store.listSessions().isEmpty();
  const bool needTransfers = hasTransferLegacy && store.loadTransfers().isEmpty();
  // A skipped source is left in place: it may still be needed (after
  // clearSessions the next start re-imports it). Archiving unread data to
  // .bak would hide it where nothing probes again.
  if(!needSessions && hasCandidate) {
    qCWarning(lmcApp(), "LegacyMigration: legacy %s skipped, database not empty",
              qPrintable(historyPath));
  }
  if(!needTransfers && hasTransferLegacy) {
    qCWarning(lmcApp(), "LegacyMigration: legacy %s skipped, database not empty",
              qPrintable(transferPath));
  }

  int importedSessions = 0;
  int importedMessages = 0;
  int skippedBlobs = 0;
  int importedTransfers = 0;
  int skippedTransfers = 0;

  if(needSessions) {
    // Whole legacy file in RAM by design: messenger.db-era files are
    // KBs/MBs, and all-or-nothing keeps a failed import retryable
    // (chunked commits would duplicate on retry: no dedup).
    QList<SessionToSave> batch;
    for(const MsgInfo& info : History::getListFrom(historyPath)) {
      const QString blob = History::getMessageFrom(historyPath, info.offset);
      SessionToSave session;
      session.name = info.name;
      session.closedAt = info.date;
      QList<HistoryRecord> records;
      if(parseStructuredHistory(blob, &records)) {
        for(const HistoryRecord& rec : records) {
          StoredMessage m;
          m.senderId = rec.userName;
          m.senderName = rec.userName;
          m.isOwn = rec.own;
          m.messageType = rec.type;
          m.createdAt = rec.ts > 0 ? rec.ts : info.date.toMSecsSinceEpoch();
          m.body = rec.body;
          session.messages.append(m);
        }
      } else {
        const QString plain = legacyHtmlToPlain(blob);
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
        session.messages.append(m);
      }
      if(session.messages.isEmpty()) {
        skippedBlobs++;
        continue;
      }
      batch.append(session);
    }
    if(!batch.isEmpty() && !store.saveSessionBatch(batch))
      return false;
    for(const SessionToSave& s : batch) {
      importedSessions++;
      importedMessages += s.messages.count();
    }
  }

  if(needTransfers) {
    FileModel model;
    model.loadData(transferPath);
    QList<TransferRecord> records;
    for(int i = 0; i < model.rowCount(); ++i) {
      FileView* v = model.item(i);
      if(!v || v->id.isEmpty()) {
        skippedTransfers++;
        continue;
      }
      TransferRecord r;
      r.id = v->id;
      r.mode = (v->mode == FileView::TM_Send || v->mode == FileView::TM_Receive)
                 ? int(v->mode)
                 : int(FileView::TM_Send);
      r.messageType = int(MT_File);
      r.filePath = v->filePath;
      r.fileDisplay = v->fileDisplay;
      r.userId = v->userId.isEmpty() ? v->userName : v->userId;
      r.userName = v->userName;
      r.fileSize = v->fileSize;
      r.state = (v->state >= 0 && v->state < int(FileView::TS_Max)) ? int(v->state)
                                                                    : int(FileView::TS_Complete);
      r.createdAt = QDateTime::currentMSecsSinceEpoch();
      records.append(r);
    }
    if(!records.isEmpty() && !store.saveTransfers(records))
      return false;
    importedTransfers = records.count();
  }

  // Acknowledge consumption only after every phase succeeded. A .bak rename
  // between the phases would leave restoreStashedLegacy with nothing to put
  // back: the fresh DB holding committed rows gets deleted on retry while
  // the source is already archived where nothing probes.
  // uniqueBackupPath: a leftover .bak never blocks the rename into an
  // endless "skipped, database not empty" loop.
  if(needSessions) {
    const QString dest = HistoryStore::uniqueBackupPath(historyPath + QStringLiteral(".bak"));
    if(!QFile::rename(historyPath, dest))
      qCWarning(lmcApp(), "LegacyMigration: cannot archive %s as %s",
                qPrintable(historyPath), qPrintable(dest));
  }
  if(needTransfers) {
    const QString dest = HistoryStore::uniqueBackupPath(transferPath + QStringLiteral(".bak"));
    if(!QFile::rename(transferPath, dest))
      qCWarning(lmcApp(), "LegacyMigration: cannot archive %s as %s",
                qPrintable(transferPath), qPrintable(dest));
  }

  qCInfo(lmcApp(),
         "LegacyMigration: legacy import sessions=%d messages=%d skippedBlobs=%d "
         "transfers=%d skippedTransfers=%d",
         importedSessions, importedMessages, skippedBlobs, importedTransfers,
         skippedTransfers);
  return true;
}

bool LegacyMigration::settleStash(const QString& dbPath,
                                  const QString& stashedLegacy, bool migrated) {
  if(migrated) {
    preserveStashLeftovers(dbPath, stashedLegacy);
    return true;
  }
  return restoreStashedLegacy(dbPath, stashedLegacy);
}

void LegacyMigration::preserveStashLeftovers(const QString& dbPath,
                                             const QString& stashedLegacy) {
  if(stashedLegacy.isEmpty())
    return;
  const QString wrongBak = stashedLegacy + QStringLiteral(".bak");
  const QString rightBak = dbPath + QStringLiteral(".bak");
  QStringList toPreserve;
  if(QFile::exists(wrongBak))
    toPreserve.append(wrongBak);
  // Timestamped stash backup from uniqueBackupPath (see migrateLegacyHistory).
  {
    const QDir dir(QFileInfo(stashedLegacy).absolutePath());
    const QString pattern = QFileInfo(wrongBak).fileName() + QStringLiteral(".*");
    const QStringList stamped =
      dir.entryList({pattern}, QDir::Files, QDir::Name);
    for(const QString& name : stamped)
      toPreserve.append(dir.absoluteFilePath(name));
  }
  if(toPreserve.isEmpty() && QFile::exists(stashedLegacy))
    toPreserve.append(stashedLegacy);
  for(const QString& source : toPreserve) {
    const QString dest = HistoryStore::uniqueBackupPath(rightBak);
    if(!QFile::rename(source, dest))
      qCWarning(lmcApp(), "LegacyMigration: cannot preserve %s as %s",
                qPrintable(source), qPrintable(dest));
  }
}
