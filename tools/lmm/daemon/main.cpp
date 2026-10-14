// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// lmmd entry point: LAN Messenger Mock daemon over QLocalServer NDJSON.
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QSocketNotifier>
#include <QTemporaryDir>
#include <csignal>
#include <unistd.h>

#include "core/definitions.h"
#include "lmmd.h"

namespace {
// Self-pipe: the only async-signal-safe way to wake the event loop.
// The handler must not touch Qt (stopAll()/quit() are not signal-safe).
int gSigFd[2] = {-1, -1};

void handleSignal(int) {
  if(gSigFd[1] >= 0) {
    const char c = 1;
    ::write(gSigFd[1], &c, sizeof(c));
  }
}

void usage() {
  qInfo("usage: lmmd [--socket-name NAME] [--home DIR] [--version]");
  qInfo("  --socket-name NAME  QLocalServer name (default: lmm)");
  qInfo("  --home DIR          HOME for peer settings/state (default: fresh temp dir)");
  qInfo("  --version           print version and exit");
}
} // namespace

int main(int argc, char* argv[]) {
  QString socketName = "lmm";
  QString homeDir;
  for(int i = 1; i < argc; i++) {
    const QString a = QString::fromLocal8Bit(argv[i]);
    if(a == "--socket-name" && i + 1 < argc)
      socketName = QString::fromLocal8Bit(argv[++i]);
    else if(a == "--home" && i + 1 < argc)
      homeDir = QString::fromLocal8Bit(argv[++i]);
    else if(a == "--version") {
      qInfo("lmmd %s (lmc %s)", LMM_VERSION, IDA_VERSION);
      return 0;
    } else {
      usage();
      return 2;
    }
  }

  // Isolate peer settings/state from the user's real ~/.config/lmc.
  QTemporaryDir tempHome;
  if(homeDir.isEmpty()) {
    if(!tempHome.isValid()) {
      qCritical("lmmd: cannot create temp HOME");
      return 1;
    }
    homeDir = tempHome.path();
  } else if(!QDir().mkpath(homeDir)) {
    qCritical("lmmd: cannot create --home dir");
    return 1;
  }
  qputenv("HOME", homeDir.toLocal8Bit());
  qputenv("XDG_CONFIG_HOME", (homeDir + "/.config").toLocal8Bit());
  qputenv("XDG_DATA_HOME", (homeDir + "/.local/share").toLocal8Bit());
  qputenv("XDG_CACHE_HOME", (homeDir + "/.cache").toLocal8Bit());
  if(qgetenv("QT_QPA_PLATFORM").isEmpty())
    qputenv("QT_QPA_PLATFORM", "offscreen");

  // QApplication parses argv; hide our custom flags from it.
  int qargc = 1;
  QApplication app(qargc, argv);
  QApplication::setApplicationName(IDA_PRODUCT);
  QApplication::setOrganizationName(IDA_COMPANY);
  QApplication::setOrganizationDomain(IDA_DOMAIN);

  if(::pipe(gSigFd) != 0) {
    qCritical("lmmd: cannot create signal pipe");
    return 1;
  }
  QSocketNotifier sigWatch(gSigFd[0], QSocketNotifier::Read);
  QObject::connect(&sigWatch, &QSocketNotifier::activated, &app, [](int fd) {
    char c;
    ::read(fd, &c, sizeof(c)); // drain
    QCoreApplication::quit();
  });

  Lmmd daemon(socketName);
  ::signal(SIGINT, handleSignal);
  ::signal(SIGTERM, handleSignal);
  // Single cleanup point: quit() (from a signal or shutdown RPC) lands here.
  QObject::connect(&app, &QCoreApplication::aboutToQuit, &daemon, &Lmmd::stopAll);

  QString error;
  if(!daemon.start(&error)) {
    qCritical("lmmd: cannot listen on %s: %s", qPrintable(socketName), qPrintable(error));
    return 1;
  }
  qInfo("lmmd: listening on %s (home %s)", qPrintable(socketName), qPrintable(homeDir));
  return app.exec();
}
