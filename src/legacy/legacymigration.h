// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
//
// One-shot legacy history import. Reads binary messenger.db / transfers.lst,
// writes through the public HistoryStore API. QtSql stays inside the store.
#pragma once

#include <QString>

class HistoryStore;

class LegacyMigration {
public:
  // Cheap startup probe: legacy at the target path or candidates present.
  static bool hasLegacyWork(const QString& dbTarget, QString* historyPath,
                            QString* transferPath);
  // Legacy messenger.db at the SQLite target path is not SQLite: move it
  // aside before open(). *stashedLegacy receives the stash path or empty.
  // A leftover stash from a crashed run is preserved, never deleted.
  static bool stashLegacyAtTarget(const QString& targetPath, QString* stashedLegacy);
  // Put the stash back for a retry. False = the put-back failed (warned);
  // the next start retries only if the stash file survived. A missing stash
  // never touches the target: deleting it would lose both sides.
  static bool restoreStashedLegacy(const QString& targetPath,
                                   const QString& stashedLegacy);
  // Settings -> candidate paths (empty when absent).
  static void legacyCandidates(QString* historyPath, QString* transferPath);
  // Import into an open store; renames consumed sources to .bak.
  static bool migrateLegacyHistory(HistoryStore& store, const QString& historyPath,
                                   const QString& transferPath);
  // Move stash leftovers next to the DB as <db>.bak (never overwrite).
  static void preserveStashLeftovers(const QString& dbPath, const QString& stashedLegacy);
  // Settle the stash after the import attempt: preserve on success, put
  // back for a retry on failure. Empty stash is a no-op. False = the
  // put-back failed (warned, see restoreStashedLegacy).
  static bool settleStash(const QString& dbPath, const QString& stashedLegacy,
                          bool migrated);
};
