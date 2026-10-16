// Test binary entry point (lan-messenger-test): thin wrapper over
// lmcRunApplication (lmc/src/app/apprunner.h, same lmc_core as production),
// plus the in-process test agent linked in directly.
// Guarded by LMC_TEST_AGENT ("0" disables the server); port from
// LMC_AGENT_PORT (default 9999, see PROTOCOL.md).
//
// Production binary stays clean: the agent lives only in this target
// behind LMC_WITH_TEST_AGENT.

#include "app/apprunner.h"
#include "agent_server.h"
#include "core/trace.h"
#include <QCoreApplication>

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

int main(int argc, char* argv[]) {
  return lmcRunApplication(argc, argv, [](QCoreApplication& application) {
    if(!testAgentEnabled())
      return;
    // The test agent server, born in the GUI thread before exec().
    // Parented to the application: freed on quit, no delete needed.
    new lmc_agent::AgentServer(testAgentPort(), lmcTrace::fileName(),
                               &application);
  });
}
