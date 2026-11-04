// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Autostart desktop entry: canonical name migration (lmc.desktop ->
// io.github.lanmessenger.lmc.desktop) and entry contents.
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QApplication>

#include "core/settings.h"
#include "core/definitions.h"

namespace {

// Restores XDG_CONFIG_HOME/HOME on destruction (QVERIFY/QSKIP safe).
struct HomeGuard {
  bool hadXdg;
  QByteArray oldXdg;
  bool hadHome;
  QByteArray oldHome;
  HomeGuard()
      : hadXdg(qEnvironmentVariableIsSet("XDG_CONFIG_HOME")),
        oldXdg(qgetenv("XDG_CONFIG_HOME")),
        hadHome(qEnvironmentVariableIsSet("HOME")),
        oldHome(qgetenv("HOME")) {
  }
  ~HomeGuard() {
    if(hadXdg)
      qputenv("XDG_CONFIG_HOME", oldXdg);
    else
      qunsetenv("XDG_CONFIG_HOME");
    if(hadHome)
      qputenv("HOME", oldHome);
    else
      qunsetenv("HOME");
  }
};

QString readAll(const QString& path) {
  QFile file(path);
  if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return QString();
  QTextStream stream(&file);
  return stream.readAll();
}

void checkEntryContent(const QString& content) {
  QVERIFY(content.startsWith(QStringLiteral("[Desktop Entry]\n")));
  QVERIFY(content.contains(QStringLiteral("\nVersion=1.0\n")));
  QVERIFY(content.contains(QStringLiteral("\nType=Application\n")));
  QVERIFY(content.contains(QStringLiteral("\nIcon=" LMC_DESKTOP_ID "\n")));
  QVERIFY(content.contains(QStringLiteral("\nStartupWMClass=lmc\n")));
  // Points at the binary that created the entry (installed: "/usr/bin/lmc").
  QVERIFY(content.contains(QStringLiteral("\nExec=\"") +
                           QDir::toNativeSeparators(QApplication::applicationFilePath()) +
                           QStringLiteral("\"\n")));
  QVERIFY(!content.contains(QStringLiteral("lmc.sh")));
}

} // namespace

class TstAutostart : public QObject {
  Q_OBJECT
private slots:
  void enableMigratesOldEntry(void);
  void disableRemovesBoth(void);
  void homeFallbackWritesUnderDotConfig(void);
  void emptyXdgFallsBackToHome(void);
  void emptyHomeStillUsesXdg(void);
  void bothEmptyFallsBackToHomePath(void);
};

void TstAutostart::enableMigratesOldEntry(void) {
#if !defined(Q_OS_UNIX) || defined(Q_OS_MAC) || defined(Q_OS_MACOS)
  QSKIP("autostart desktop entry is Linux-only");
#endif
  HomeGuard guard;
  QTemporaryDir home;
  QVERIFY(home.isValid());
  QDir().mkpath(home.path() + QStringLiteral("/autostart"));
  qputenv("XDG_CONFIG_HOME", home.path().toUtf8());

  const QString oldPath = home.path() + QStringLiteral("/autostart/lmc.desktop");
  const QString newPath =
    home.path() + QStringLiteral("/autostart/" LMC_DESKTOP_ID ".desktop");
  {
    QFile old(oldPath);
    QVERIFY(old.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&old);
    stream << "[Desktop Entry]\nType=Application\nIcon=lmc\n";
  }

  lmcSettings::setAutoStart(true);

  QVERIFY(!QFile::exists(oldPath));
  QVERIFY(QFile::exists(newPath));
  checkEntryContent(readAll(newPath));
}

void TstAutostart::disableRemovesBoth(void) {
#if !defined(Q_OS_UNIX) || defined(Q_OS_MAC) || defined(Q_OS_MACOS)
  QSKIP("autostart desktop entry is Linux-only");
#endif
  HomeGuard guard;
  QTemporaryDir home;
  QVERIFY(home.isValid());
  QDir().mkpath(home.path() + QStringLiteral("/autostart"));
  qputenv("XDG_CONFIG_HOME", home.path().toUtf8());

  const QString oldPath = home.path() + QStringLiteral("/autostart/lmc.desktop");
  const QString newPath =
    home.path() + QStringLiteral("/autostart/" LMC_DESKTOP_ID ".desktop");
  {
    QFile old(oldPath);
    QVERIFY(old.open(QIODevice::WriteOnly | QIODevice::Text));
    old.write("[Desktop Entry]\n");
  }
  {
    QFile cur(newPath);
    QVERIFY(cur.open(QIODevice::WriteOnly | QIODevice::Text));
    cur.write("[Desktop Entry]\n");
  }

  lmcSettings::setAutoStart(false);

  QVERIFY(!QFile::exists(oldPath));
  QVERIFY(!QFile::exists(newPath));
}

void TstAutostart::homeFallbackWritesUnderDotConfig(void) {
#if !defined(Q_OS_UNIX) || defined(Q_OS_MAC) || defined(Q_OS_MACOS)
  QSKIP("autostart desktop entry is Linux-only");
#endif
  HomeGuard guard;
  QTemporaryDir home;
  QVERIFY(home.isValid());
  qunsetenv("XDG_CONFIG_HOME");
  qputenv("HOME", home.path().toUtf8());

  const QString newPath = home.path() +
                          QStringLiteral("/.config/autostart/" LMC_DESKTOP_ID ".desktop");

  lmcSettings::setAutoStart(true);

  QVERIFY(QFile::exists(newPath));
  checkEntryContent(readAll(newPath));

  lmcSettings::setAutoStart(false);
  QVERIFY(!QFile::exists(newPath));
}

void TstAutostart::emptyXdgFallsBackToHome(void) {
#if !defined(Q_OS_UNIX) || defined(Q_OS_MAC) || defined(Q_OS_MACOS)
  QSKIP("autostart desktop entry is Linux-only");
#endif
  HomeGuard guard;
  QTemporaryDir home;
  QVERIFY(home.isValid());
  qputenv("XDG_CONFIG_HOME", "");
  qputenv("HOME", home.path().toUtf8());

  const QString newPath = home.path() +
                          QStringLiteral("/.config/autostart/" LMC_DESKTOP_ID ".desktop");

  lmcSettings::setAutoStart(true);

  QVERIFY(QFile::exists(newPath));
  checkEntryContent(readAll(newPath));

  lmcSettings::setAutoStart(false);
  QVERIFY(!QFile::exists(newPath));
}

void TstAutostart::emptyHomeStillUsesXdg(void) {
#if !defined(Q_OS_UNIX) || defined(Q_OS_MAC) || defined(Q_OS_MACOS)
  QSKIP("autostart desktop entry is Linux-only");
#endif
  HomeGuard guard;
  QTemporaryDir xdg;
  QVERIFY(xdg.isValid());
  QDir().mkpath(xdg.path() + QStringLiteral("/autostart"));
  qputenv("XDG_CONFIG_HOME", xdg.path().toUtf8());
  qputenv("HOME", "");

  const QString newPath =
    xdg.path() + QStringLiteral("/autostart/" LMC_DESKTOP_ID ".desktop");

  lmcSettings::setAutoStart(true);

  QVERIFY(QFile::exists(newPath));
  checkEntryContent(readAll(newPath));

  lmcSettings::setAutoStart(false);
  QVERIFY(!QFile::exists(newPath));
}

void TstAutostart::bothEmptyFallsBackToHomePath(void) {
  //  Pure resolver: no setAutoStart, no registry/.desktop writes and no
  //  filesystem access, so this slot intentionally runs on all platforms
  //  (no QSKIP here) and covers the QDir::homePath() branch without
  //  touching the real home.
  QCOMPARE(lmcSettings::resolveAutoStartDir(QStringLiteral("/xdg"), QStringLiteral("/home"),
                                            QStringLiteral("/fallback")),
           QStringLiteral("/xdg/autostart"));
  QCOMPARE(lmcSettings::resolveAutoStartDir(QString(), QStringLiteral("/home"),
                                            QStringLiteral("/fallback")),
           QStringLiteral("/home/.config/autostart"));
  QCOMPARE(lmcSettings::resolveAutoStartDir(QString(), QString(), QStringLiteral("/fallback")),
           QStringLiteral("/fallback/.config/autostart"));
  //  Per XDG: whitespace-only or relative XDG_CONFIG_HOME is ignored.
  QCOMPARE(lmcSettings::resolveAutoStartDir(QStringLiteral("   "), QStringLiteral("/home"),
                                            QStringLiteral("/fallback")),
           QStringLiteral("/home/.config/autostart"));
  QCOMPARE(lmcSettings::resolveAutoStartDir(QStringLiteral("rel/path"), QStringLiteral("/home"),
                                            QStringLiteral("/fallback")),
           QStringLiteral("/home/.config/autostart"));
  //  Same strictness for HOME: whitespace-only or relative values fall
  //  through to the fallback instead of producing a relative path.
  QCOMPARE(lmcSettings::resolveAutoStartDir(QString(), QStringLiteral("   "),
                                            QStringLiteral("/fallback")),
           QStringLiteral("/fallback/.config/autostart"));
  QCOMPARE(lmcSettings::resolveAutoStartDir(QString(), QStringLiteral("rel/path"),
                                            QStringLiteral("/fallback")),
           QStringLiteral("/fallback/.config/autostart"));
  QCOMPARE(lmcSettings::resolveAutoStartDir(QStringLiteral("   "), QStringLiteral("rel/path"),
                                            QStringLiteral("/fallback")),
           QStringLiteral("/fallback/.config/autostart"));

  HomeGuard guard;
  QTemporaryDir xdgDir;
  QVERIFY(xdgDir.isValid());
  QTemporaryDir homeDir;
  QVERIFY(homeDir.isValid());

  //  Valid XDG wins over HOME: compares against dirs we control, so this
  //  proves systemAutoStartDir() actually reads the environment instead of
  //  just echoing QDir::homePath().
  qputenv("XDG_CONFIG_HOME", xdgDir.path().toUtf8());
  qputenv("HOME", homeDir.path().toUtf8());
  QCOMPARE(lmcSettings::systemAutoStartDir(), xdgDir.path() + QStringLiteral("/autostart"));

  //  Empty/whitespace/relative XDG falls back to HOME (also env-read proof).
  qunsetenv("XDG_CONFIG_HOME");
  QCOMPARE(lmcSettings::systemAutoStartDir(),
           homeDir.path() + QStringLiteral("/.config/autostart"));
  qputenv("XDG_CONFIG_HOME", "");
  QCOMPARE(lmcSettings::systemAutoStartDir(),
           homeDir.path() + QStringLiteral("/.config/autostart"));
  qputenv("XDG_CONFIG_HOME", "   ");
  QCOMPARE(lmcSettings::systemAutoStartDir(),
           homeDir.path() + QStringLiteral("/.config/autostart"));
  qputenv("XDG_CONFIG_HOME", "rel/path");
  QCOMPARE(lmcSettings::systemAutoStartDir(),
           homeDir.path() + QStringLiteral("/.config/autostart"));

  //  Bogus HOME is ignored: must fall back to a real absolute home, never
  //  build a path from whitespace or a relative value. No exact QCOMPARE
  //  here: the fallback (passwd entry) is environment-dependent, so assert
  //  the properties that matter (absolute, correct suffix, bogus prefix
  //  not leaked into the result).
  qunsetenv("XDG_CONFIG_HOME");
  qputenv("HOME", "   ");
  {
    const QString result = lmcSettings::systemAutoStartDir();
    QVERIFY(QDir::isAbsolutePath(result));
    QVERIFY(result.endsWith(QStringLiteral("/.config/autostart")));
    QVERIFY(result != QStringLiteral("   /.config/autostart"));
    QVERIFY(!result.startsWith(QStringLiteral("   ")));
  }
  qputenv("HOME", "rel/path");
  {
    const QString result = lmcSettings::systemAutoStartDir();
    QVERIFY(QDir::isAbsolutePath(result));
    QVERIFY(result.endsWith(QStringLiteral("/.config/autostart")));
    QVERIFY(result != QStringLiteral("rel/path/.config/autostart"));
    QVERIFY(!result.startsWith(QStringLiteral("rel/path")));
  }

  qunsetenv("XDG_CONFIG_HOME");
  qunsetenv("HOME");
  QCOMPARE(lmcSettings::systemAutoStartDir(),
           QDir::homePath() + QStringLiteral("/.config/autostart"));

  qputenv("XDG_CONFIG_HOME", "");
  qputenv("HOME", "");
  QCOMPARE(lmcSettings::systemAutoStartDir(),
           QDir::homePath() + QStringLiteral("/.config/autostart"));
}

QTEST_MAIN(TstAutostart)
#include "tst_autostart.moc"
