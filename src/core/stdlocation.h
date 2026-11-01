// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

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

  static QString libDir(void) {
    return QDir::toNativeSeparators(QDir::currentPath());
  }

  static QString resourceFile(void) {
    return QDir::toNativeSeparators(QDir::current().absoluteFilePath(SL_RESOURCE));
  }

  static QString resLangDir(void) {
    return ":/" SL_LANGDIR;
  }

  static QString sysLangDir(void) {
    return QDir::toNativeSeparators(QDir::currentPath() + "/" SL_LANGDIR);
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
