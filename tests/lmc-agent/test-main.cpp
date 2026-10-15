// Phase B entry point (test binary lan-messenger-test): same startup as
// lmc/src/app/main.cpp, plus the in-process test agent linked in directly
// (no LD_PRELOAD). Guarded by LMC_TEST_AGENT ("0" disables the server);
// port from LMC_AGENT_PORT (default 9999, see tests/lmc-agent/PROTOCOL.md).
//
// Production binary stays clean: this file is only compiled into the
// lan-messenger-test target behind LMC_WITH_TEST_AGENT.

#include "application.h"
#include "app/lmc.h"
#include "core/stdlocation.h"
#include "agent_server.h"
#include <QResource>
#include <QMessageBox>
#include <QCoreApplication>

// Keep in sync with lmc/src/app/main.cpp: the test binary is a drop-in
// replacement for E2E, so it shares the single-instance id.
const QString appId = "93fab548-2cf5-4a1e-8758-a416a5ec2120-6fc5009f-84e8-4489-a444-7f934bcf9166";

namespace {

bool testAgentEnabled() {
  const QString value =
    QString::fromLocal8Bit(qgetenv("LMC_TEST_AGENT")).trimmed().toLower();
  return value != QStringLiteral("0") && value != QStringLiteral("off") &&
         value != QStringLiteral("no");
}

quint16 testAgentPort() {
  bool ok = false;
  const uint parsed =
    QString::fromLocal8Bit(qgetenv("LMC_AGENT_PORT")).trimmed().toUInt(&ok);
  if(ok && parsed > 0 && parsed < 65536)
    return quint16(parsed);
  return 9999;
}

} // namespace

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
                "/? - Display this help.\n"
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

int main(int argc, char* argv[]) {
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
      return application.isRunning() ? 1 : 0;
    else
      messageList += arguments.at(index) + "\n";
  }

  if(application.sendMessage(messageList))
    return 0;

  application.loadTranslations(StdLocation::resLangDir());
  application.loadTranslations(StdLocation::sysLangDir());
  application.loadTranslations(StdLocation::userLangDir());

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
  messageList += "/trace\n";
#endif

  lmcCore core;
  core.init(messageList);
  messageList += "/new\n";
  if(!core.receiveAppMessage(messageList))
    return 0;
  if(!core.start())
    return 1;

#ifdef QT_NO_SSL
  if(QMessageBox::critical(nullptr, IDA_TITLE, "Qt is compiled with QT_NO_SSL. Some functions will not work correctly. Quit application?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    return 2;
#else
  if(!QSslSocket::supportsSsl()) {
    if(QMessageBox::critical(nullptr, IDA_TITLE, "Messenger does not find ssl at startup. Probably missing openssl dll. Some functions will not work correctly. Quit application?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      return 3;
  }
#endif

  // The test agent server, born in the GUI thread before exec().
  lmc_agent::AgentServer* agent = nullptr;
  if(testAgentEnabled())
    agent = new lmc_agent::AgentServer(testAgentPort(), &application);
  Q_UNUSED(agent);

  QObject::connect(&application, &QtSingleApplication::messageReceived, &core, &lmcCore::receiveAppMessage);
  QObject::connect(&application, &QCoreApplication::aboutToQuit, &core, &lmcCore::aboutToExit);

  return application.exec();
}
