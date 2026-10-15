// LD_PRELOAD entry (phase A): starts the agent server inside the GUI
// thread of the unmodified production binary.
//
// The constructor must not touch Qt: the host application may not exist
// yet. It only spawns a detached watcher thread; the watcher waits for
// QCoreApplication::instance and then creates the AgentServer via a
// queued invocation, so the QTcpServer is born and lives in the GUI
// thread. Port from LMC_AGENT_PORT (default 9999, see PROTOCOL.md).

#include <QCoreApplication>
#include <QMetaObject>
#include <QString>
#include <QThread>

#include <chrono>
#include <cstdlib>
#include <thread>

#include "agent_server.h"

namespace {

quint16 agentPort() {
  quint16 port = 9999;
  if(const char* env = std::getenv("LMC_AGENT_PORT")) {
    bool ok = false;
    const uint parsed = QString::fromLatin1(env).toUInt(&ok);
    if(ok && parsed > 0 && parsed < 65536)
      port = quint16(parsed);
  }
  return port;
}

void watchAndStart() {
  for(int i = 0; i < 600 && !QCoreApplication::instance(); ++i)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  QCoreApplication* app = QCoreApplication::instance();
  if(!app)
    return;
  const quint16 port = agentPort();
  QMetaObject::invokeMethod(
    app,
    [app, port]() {
      // Parented to the application: dies with it, lives in its thread.
      new lmc_agent::AgentServer(port, app);
    },
    Qt::QueuedConnection);
}

} // namespace

__attribute__((constructor)) static void lmcAgentPreloadInit() {
  std::thread(watchAndStart).detach();
}
