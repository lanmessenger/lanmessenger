// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "apprunner.h"
#include "application.h"
#include "lmc.h"
#include "core/stdlocation.h"
#include "core/uidefinitions.h"
#include "core/lmccategories.h"
#include "core/lmclogging.h"
#include <QResource>
#include <QMessageBox>
#include <QIcon>
#include <QSslSocket>
#include <QDir>
#include <cstdio>

//	Define a unique application id. This is a combination of two GUIDs
const QString appId = "93fab548-2cf5-4a1e-8758-a416a5ec2120-6fc5009f-84e8-4489-a444-7f934bcf9166";

int showSwitches(void) {
  QString msg = "Usage:	lmc [switches]\n"
                "	All switches are optional.\n"
                "\n"
                "/loopback - Allows loopback communication with local machine.\n"
                "/nohistory - Deletes existing history.\n"
                "/nofilehistory - Deletes existing file transfer history.\n"
                "/noconfig - Resets preferences to default values.\n"
                "/sync - Synchronizes application settings and their external dependencies.\n"
                "/unsync - Removes all external application dependencies.\n"
                "/term - Closes an instance which is already running.\n"
                "/inst - Returns a value indicating whether an instance is already running.\n"
                "/quit - Closes once the command line parameters are processed.\n"
                "/trace - Enables debug logging to logs/lmc_*.log (disabled by default).\n"
                "/? - Display this help.\n"
                "\n"
                "If an instance is already running, a second launch only brings its window\n"
                "to the front (or closes it with /term); other switches apply at startup only.\n"
                "\n"
                "Example:\n"
                "Start in loopback mode with default settings: lmc /loopback /noconfig\n"
                "\n"
                "Some command line switches are mutually exclusive. If multiple switches\n"
                "are specified, they will take precedence in the order given above.\n"
                "Copyright (C) 2010-2012 Qualia Digital Solutions.\n";
  QMessageBox::information(NULL, IDA_TITLE, msg, QMessageBox::Ok);
  return 0;
}

static void initTrace(const QStringList& arguments) {
  if(!arguments.contains("/trace", Qt::CaseInsensitive)) {
    LmcLogging::disable();
    return;
  }
  LmcLogging::enable(StdLocation::freeLogFile());
}

// IPC is two commands: "raise" (default, brings the first instance forward)
// and "term" (closes it; sent by the NSIS uninstaller and the deb smoke).
// Other switches are startup-only and never leave this process: a second
// launch never applies them to the running instance.
QString remoteCommandFor(const QStringList& arguments) {
  if(arguments.contains("/term", Qt::CaseInsensitive))
    return QStringLiteral("term");
  return QStringLiteral("raise");
}

int lmcRunApplication(int argc, char* argv[],
                      const std::function<void(QCoreApplication&)>& preExecHook) {
  Application application(appId, argc, argv);
  QDir::setCurrent(QApplication::applicationDirPath());

#ifdef Q_OS_MAC
  QDir dir(QApplication::applicationDirPath());
  dir.cdUp();
  dir.cd("Plugins");
  QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif
  QResource::registerResource(StdLocation::resourceFile());

  QApplication::setApplicationName(IDA_PRODUCT);
  QApplication::setOrganizationName(IDA_COMPANY);
  QApplication::setOrganizationDomain(IDA_DOMAIN);
  QApplication::setWindowIcon(QIcon(IDR_APPICON));

  QString messageList;
  QStringList arguments = QApplication::arguments();

  for(int index = 0; index < arguments.count(); index++) {
    if(arguments.at(index).compare("/?", Qt::CaseInsensitive) == 0)
      return showSwitches();
    else if(arguments.at(index).compare("/inst", Qt::CaseInsensitive) == 0)
      // 1 iff the single-instance lock is held. Pathology never holds it.
      return application.isRunning() ? 1 : 0;
    else
      messageList += arguments.at(index) + "\n";
  }

  // Probe once, then branch: second delivers and exits, first serves,
  // pathology degrades.
  const SingleInstance::StartupOutcome outcome =
    application.resolveStartup(remoteCommandFor(arguments), 10, 200);
  if(outcome == SingleInstance::StartupOutcome::SecondDelivered)
    return 0;

  initTrace(arguments);
  if(outcome == SingleInstance::StartupOutcome::Pathology) {
    fprintf(stderr, "lmc: warning: single-instance server unavailable, continuing without it\n");
    qCWarning(lmcApp(), "Startup: single-instance pathology, continuing degraded");
  } else {
    qCInfo(lmcApp(), "Startup: first instance, lock acquired");
  }

  application.loadTranslations(StdLocation::resLangDir());
  application.loadTranslations(StdLocation::sysLangDir());
  application.loadTranslations(StdLocation::userLangDir());

  lmcCore core;
  //	handle command line args if this is first instance
  //	some args are handled when the application is initializing
  core.init(messageList);
  //	the remaining args are handled after initializing all the layers
  if(!core.processStartupSwitches(messageList)) {
    qCInfo(lmcApp(), "Startup: exiting before event loop (/quit)");
    return 0;
  }
  if(!core.start()) {
    qCInfo(lmcApp(), "Startup: core start failed, exiting");
    return 1;
  }

#ifdef QT_NO_SSL
  qCCritical(lmcApp(), "Startup: Qt compiled with QT_NO_SSL, prompting to quit");
  if(QMessageBox::critical(nullptr, IDA_TITLE, "Qt is compiled with QT_NO_SSL. Some functions will not work correctly. Quit application?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    return 2;
#else
  if(!QSslSocket::supportsSsl()) {
    qCCritical(lmcApp(), "Startup: SSL backend missing, prompting to quit");
    if(QMessageBox::critical(nullptr, IDA_TITLE, "Messenger does not find ssl at startup. Probably missing openssl dll. Some functions will not work correctly. Quit application?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      return 3;
  }
#endif

  // Test hook: the test binary starts its agent server here.
  if(preExecHook)
    preExecHook(application);

  QObject::connect(&application, &Application::messageReceived, &core, &lmcCore::receiveRemoteMessage);
  QObject::connect(&application, &QCoreApplication::aboutToQuit, &core, &lmcCore::aboutToExit);

  qCInfo(lmcApp(), "Startup: entering event loop");
  int result = application.exec();
  qCInfo(lmcApp(), "Shutdown: event loop exited with code %d", result);

  return result;
}
