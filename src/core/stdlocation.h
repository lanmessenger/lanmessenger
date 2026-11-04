// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include <QDateTime>
#include <QStandardPaths>
#include "settings.h"

#define SL_TRANSFERHISTORY "transfers.lst"
#define SL_HISTORYDB "lanmessenger.db"
#define SL_FILESTORAGEDIR "Received Files"
#define SL_CACHEDIR "cache"
#define SL_RESOURCE "lmc.rcc"
#define SL_LANGDIR "lang"
#define SL_SOUNDDIR "sounds"
#define SL_THEMEDIR "themes"
#define SL_GROUPFILE "group.cfg"
#define SL_AVATARFILE "avt_local.png"
#define SL_LOGDIR "logs"
#define SL_TEMPCONFIG "lmctmpconf.ini"

class StdLocation {
public:
  // Empty = nothing to migrate (no custom value, different file name, or
  // the file exists: a legacy binary is consumed by the stash flow, an
  // existing SQLite DB must not orphan).
  static QString migratedDbName(const QString& custom) {
    if(custom.isEmpty())
      return QString();
    if(QFileInfo(custom).fileName().compare(QStringLiteral("messenger.db"),
                                            Qt::CaseInsensitive) != 0)
      return QString();
    if(QFile::exists(custom))
      return QString();
    // Legacy-named value with nothing there yet: resolve to the new name.
    return custom.left(custom.length() - QStringLiteral("messenger.db").length()) +
           QStringLiteral(SL_HISTORYDB);
  }

  static QString transferHistory(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_TRANSFERHISTORY);
  }

  static QString historyDatabase(void) {
    QString path = QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                              QStandardPaths::AppLocalDataLocation) +
                                            "/" SL_HISTORYDB);
    lmcSettings settings;
    if(settings.value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool())
      return path;
    QString custom = settings.value(IDS_HISTORYPATH, path).toString();
    if(custom.isEmpty())
      return path;
    // Pure resolver, no writes: persistence is migrateHistoryDbName's job.
    const QString migrated = migratedDbName(custom);
    return migrated.isEmpty() ? custom : migrated;
  }

  // One-shot persistence of the legacy-named setting (see historyDatabase).
  static void migrateHistoryDbName(void) {
    lmcSettings settings;
    if(settings.value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool())
      return;
    const QString migrated =
      migratedDbName(settings.value(IDS_HISTORYPATH).toString());
    if(!migrated.isEmpty())
      settings.setValue(IDS_HISTORYPATH, migrated);
  }

  static QString fileStorageDir(void) {
    lmcSettings settings;

    QString path = QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                              QStandardPaths::DocumentsLocation) +
                                            "/" SL_FILESTORAGEDIR);
    path = settings.value(IDS_FILESTORAGEPATH, path).toString();
    return path;
  }

  static QString cacheDir(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_CACHEDIR);
  }

  // appDir -> appDir/../share/lmc (reloc, Unix-only) -> GenericDataLocation + "/lmc".
  // AppDataLocation gives .../lmc/lmc, so it is not used.
  static QStringList dataCandidateDirs(void) {
    QStringList bases;
    const QString appDir = QCoreApplication::applicationDirPath();
    if(!appDir.isEmpty()) {
      bases << QDir::cleanPath(appDir);
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
      bases << QDir::cleanPath(appDir + "/../share/lmc");
#endif
    }
    // NB: AppDataLocation resolves to .../lmc/lmc with org/app lmc/lmc.
    const QStringList dataDirs =
      QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for(const QString& dir : dataDirs) {
      const QString cand = QDir::cleanPath(dir + "/lmc");
      if(!bases.contains(cand))
        bases << cand;
    }
    return bases;
  }

  // First existing candidate, else the first one.
  static QString firstExistingOrFirst(const QStringList& candidates) {
    for(const QString& cand : candidates) {
      if(QFile::exists(cand))
        return QDir::toNativeSeparators(cand);
    }
    if(!candidates.isEmpty())
      return QDir::toNativeSeparators(candidates.first());
    return QString();
  }

  static QString resourceFile(void) {
    QStringList candidates;
    for(const QString& base : dataCandidateDirs())
      candidates << QDir::cleanPath(base + "/" SL_RESOURCE);
    return firstExistingOrFirst(candidates);
  }

  // Only the known legacy prefixes collapse to the basename; anything
  // else relative is left untouched.
  static QString normalizeSoundFile(const QString& stored) {
    if(stored.isEmpty())
      return stored;
    if(QDir::isAbsolutePath(stored))
      return QDir::toNativeSeparators(stored);
    QString base = stored;
    for(const QString& prefix : {QStringLiteral("./sounds/"), QStringLiteral("sounds/")}) {
      if(base.startsWith(prefix)) {
        base.remove(0, prefix.length());
        break;
      }
    }
    if(base.isEmpty() || base.contains(QLatin1Char('/')) || base.contains(QLatin1Char('\\')))
      return stored;
    return base;
  }

  static QString resolveSoundFile(const QString& stored) {
    return resolveSoundFileFrom(stored, soundCandidateDirs());
  }

  static QString resolveSoundFileFrom(const QString& stored, const QStringList& soundDirs) {
    if(stored.isEmpty())
      return stored;
    if(QDir::isAbsolutePath(stored))
      return QDir::toNativeSeparators(stored);
    const QString base = normalizeSoundFile(stored);
    QStringList candidates;
    for(const QString& dir : soundDirs)
      candidates << QDir::cleanPath(dir + "/" + base);
    return firstExistingOrFirst(candidates);
  }

  static QStringList soundCandidateDirs(void) {
    QStringList dirs;
    for(const QString& base : dataCandidateDirs())
      dirs << QDir::cleanPath(base + "/" SL_SOUNDDIR);
    return dirs;
  }

  static QString resLangDir(void) {
    return ":/" SL_LANGDIR;
  }

  static QString sysLangDir(void) {
    QStringList candidates;
    for(const QString& base : dataCandidateDirs())
      candidates << QDir::cleanPath(base + "/" SL_LANGDIR);
    return firstExistingOrFirst(candidates);
  }

  static QString userLangDir(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_LANGDIR);
  }

  static QString userThemeDir(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_THEMEDIR);
  }

  static QString groupFile(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_GROUPFILE);
  }

  static QString avatarFile(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_AVATARFILE);
  }

  static QString logDir(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::AppLocalDataLocation) +
                                    "/" SL_LOGDIR);
  }

  static QString freeLogFile(void) {
    QString fileName = "lmc_" +
                       QString::number(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()) + ".log";
    return QDir::toNativeSeparators(logDir() + "/" + fileName);
  }

  static QString tempConfigFile(void) {
    return QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                      QStandardPaths::TempLocation) +
                                    "/" SL_TEMPCONFIG);
  }
};
