// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// lmmd entry point: supervisor by default, hidden --worker mode per peer.
// External face is one QTcpServer NDJSON socket on 127.0.0.1 (see PROTOCOL.md).
#include <QCoreApplication>
#include <QDir>
#include <QJsonObject>
#include <QSettings>
#include <QSocketNotifier>
#include <QTemporaryDir>
#include <QTextStream>
#include <csignal>
#ifdef Q_OS_WIN
// No <unistd.h> here: the self-pipe below is POSIX-only (and
// QSocketNotifier cannot watch pipe fds on Windows anyway). The Windows
// CRT raises console Ctrl handlers for SIGINT/SIGBREAK, and quit() only
// posts to the event loop, so no pipe is needed there.
#else
#include <unistd.h>
#endif

#include "core/definitions.h"
#include "lmmd.h"
#include "supervisor.h"

namespace {
// Self-pipe: the only async-signal-safe way to wake the event loop.
// The handler must not touch Qt (stopAll()/quit() are not signal-safe).
#ifndef Q_OS_WIN
int gSigFd[2] = {-1, -1};
#endif

void handleSignal(int) {
#ifdef Q_OS_WIN
  // Runs on a Windows system thread for console Ctrl events; quit() only
  // posts to the event loop, so this is safe without a self-pipe.
  QCoreApplication::quit();
#else
  if(gSigFd[1] >= 0) {
    const char c = 1;
    ::write(gSigFd[1], &c, sizeof(c));
  }
#endif
}

void usage() {
  qInfo("usage: lmmd [--port N] [--home DIR] [--version]");
  qInfo("  --port N   TCP loopback port (default: $LMMD_PORT or 17777)");
  qInfo("  --home DIR          base HOME for peer state (default: fresh temp dir)");
  qInfo("  --version           print version and exit");
  qInfo("hidden worker mode (spawned by the supervisor, not for manual use):");
  qInfo("  lmmd --worker --peer NAME --home DIR [--status S]");
}

// HOME + XDG_* isolation. Shared by supervisor (base dir) and worker
// (per-peer dir <base>/peers/<name>/, passed by the supervisor).
// Empty homeDir => fresh temp dir, auto-removed at process exit.
bool setupHome(const QString& homeDir, QString* resolved) {
  QString home = homeDir;
  if(home.isEmpty()) {
    static QTemporaryDir tempHome; // destroyed at exit => dir removed
    if(!tempHome.isValid())
      return false;
    home = tempHome.path();
  } else if(!QDir().mkpath(home)) {
    return false;
  }
  qputenv("HOME", home.toLocal8Bit());
  qputenv("XDG_CONFIG_HOME", (home + "/.config").toLocal8Bit());
  qputenv("XDG_DATA_HOME", (home + "/.local/share").toLocal8Bit());
  qputenv("XDG_CACHE_HOME", (home + "/.cache").toLocal8Bit());
  if(resolved)
    *resolved = home;
  return true;
}

bool setupSignals(QCoreApplication* app) {
#ifdef Q_OS_WIN
  (void)app;
  ::signal(SIGINT, handleSignal);
  ::signal(SIGTERM, handleSignal);
  ::signal(SIGBREAK, handleSignal);
  return true;
#else
  if(::pipe(gSigFd) != 0) {
    qCritical("lmmd: cannot create signal pipe");
    return false;
  }
  auto* sigWatch = new QSocketNotifier(gSigFd[0], QSocketNotifier::Read, app);
  QObject::connect(sigWatch, &QSocketNotifier::activated, app, [](int fd) {
    char c;
    ::read(fd, &c, sizeof(c)); // drain
    QCoreApplication::quit();
  });
  ::signal(SIGINT, handleSignal);
  ::signal(SIGTERM, handleSignal);
  return true;
#endif
}

void setupApp(QCoreApplication* app) {
  app->setApplicationName(IDA_PRODUCT);
  app->setOrganizationName(IDA_COMPANY);
  app->setOrganizationDomain(IDA_DOMAIN);
}

int runWorker(int argc, char** argv, const QString& peer, const QString& homeDir,
              const QString& status) {
  // Identity: userId = MAC + logon name, so the worker owns its name.
  // USER covers Unix (Helper::getLogonName reads getenv there);
  // LMC_LOGON_NAME covers Windows, where GetUserName reads the process
  // token and ignores the environment (else all workers collide).
  qputenv("USER", peer.toLatin1());
  qputenv("LMC_LOGON_NAME", peer.toLatin1());
  QString home;
  if(!setupHome(homeDir, &home)) {
    qCritical("lmmd worker: cannot create --home dir");
    return 1;
  }
#ifdef Q_OS_WIN
  // QSettings IniFormat ignores HOME/XDG on Windows (one per-user
  // location), so without this all workers share a single settings file:
  // no isolation, cross-talk on Name/Status. Pin this worker to its own
  // peer dir. Unix keeps the XDG layout (untouched).
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home);
#endif
  int qargc = 1; // hide our custom flags from Qt
  QCoreApplication app(qargc, argv);
  setupApp(&app);
  if(!setupSignals(&app))
    return 1;

  Lmmd daemon(0); // ephemeral port; the supervisor reads it back below
  daemon.setSinglePeer(true);
  QObject::connect(&app, &QCoreApplication::aboutToQuit, &daemon, &Lmmd::stopAll);

  QString error;
  if(!daemon.start(&error)) {
    qCritical("lmmd worker: cannot listen: %s", qPrintable(error));
    return 1;
  }
  // The worker holds exactly one peer, added up front. No peer handles
  // anywhere else: control commands are self-scoped by construction.
  QJsonObject params{{"name", peer}};
  if(!status.isEmpty())
    params["status"] = status;
  QJsonObject res =
      daemon.handleRequest(QJsonObject{{"id", 1}, {"cmd", "peer.add"}, {"params", params}});
  if(!res.value("ok").toBool()) {
    qCritical("lmmd worker: peer.add failed: %s", qPrintable(res.value("error").toString()));
    return 1;
  }
  // Rendezvous for the supervisor (stdout; logs go to stderr): the only
  // line in this stream, read before the first control connection.
  QTextStream out(stdout);
  out << "READY " << daemon.serverPort() << "\n";
  out.flush();
  qInfo("lmmd worker %s: listening on 127.0.0.1:%u (home %s)", qPrintable(peer),
        daemon.serverPort(), qPrintable(home));
  return app.exec();
}

int runSupervisor(int argc, char** argv, quint16 port, const QString& homeDir) {
  QString home;
  if(!setupHome(homeDir, &home)) {
    qCritical("lmmd: cannot create --home dir");
    return 1;
  }
  if(!QDir().mkpath(home + "/peers")) {
    qCritical("lmmd: cannot create peers dir");
    return 1;
  }
  int qargc = 1; // hide our custom flags from Qt
  QCoreApplication app(qargc, argv);
  setupApp(&app);
  if(!setupSignals(&app))
    return 1;

  Supervisor daemon(port, home);
  // Single cleanup point: quit() (signal, shutdown RPC, fatal) lands here.
  QObject::connect(&app, &QCoreApplication::aboutToQuit, &daemon, [&daemon]() {
    daemon.shutdown(daemon.exitCode());
  });

  QString error;
  if(!daemon.start(&error)) {
    qCritical("lmmd: cannot listen on 127.0.0.1:%u: %s", port, qPrintable(error));
    return 1;
  }
  qInfo("lmmd: listening on 127.0.0.1:%u (home %s)", port, qPrintable(home));
  app.exec();
  return daemon.exitCode();
}
} // namespace

int main(int argc, char* argv[]) {
  quint16 port = 0; // 0 = unset; resolved below (CLI > $LMMD_PORT > default)
  QString homeDir;
  bool worker = false;
  QString peer, status;
  for(int i = 1; i < argc; i++) {
    const QString a = QString::fromLocal8Bit(argv[i]);
    if(a == "--port" && i + 1 < argc) {
      bool ok = false;
      port = QString::fromLocal8Bit(argv[++i]).toUShort(&ok);
      if(!ok || port == 0) {
        usage();
        return 2;
      }
    } else if(a == "--home" && i + 1 < argc)
      homeDir = QString::fromLocal8Bit(argv[++i]);
    else if(a == "--worker")
      worker = true;
    else if(a == "--peer" && i + 1 < argc)
      peer = QString::fromLocal8Bit(argv[++i]);
    else if(a == "--status" && i + 1 < argc)
      status = QString::fromLocal8Bit(argv[++i]);
    else if(a == "--version") {
      qInfo("lmmd %s (lmc %s)", LMM_VERSION, IDA_VERSION);
      return 0;
    } else {
      usage();
      return 2;
    }
  }

  if(worker) {
    if(port != 0 || peer.isEmpty() || homeDir.isEmpty()) {
      usage();
      return 2;
    }
    return runWorker(argc, argv, peer, homeDir, status);
  }
  if(!peer.isEmpty() || !status.isEmpty()) {
    usage();
    return 2;
  }
  if(port == 0) {
    bool ok = false;
    port = QString::fromLocal8Bit(qgetenv("LMMD_PORT")).toUShort(&ok);
    if(!ok || port == 0)
      port = 17777;
  }
  return runSupervisor(argc, argv, port, homeDir);
}
