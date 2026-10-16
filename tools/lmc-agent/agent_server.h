#ifndef LMC_AGENT_SERVER_H
#define LMC_AGENT_SERVER_H

// QTcpServer speaking the NDJSON protocol from PROTOCOL.md.
// Lives in the GUI thread (parented to QApplication or the test core):
// every command handler runs there, so widget access needs no marshaling.
//
// While a command spins its bounded local event loop the server reports
// "busy" to any pipelined second request (the Robot library never
// pipelines, so this is a reentrancy guard, not a limit).

#include <QObject>
#include <QString>
#include <QTcpServer>

namespace lmc_agent {

class AgentServer : public QTcpServer {
  Q_OBJECT

public:
  explicit AgentServer(quint16 port, const QString& appLogPath,
                       QObject* parent = nullptr);

  quint16 port() const {
    return serverPort();
  }

  bool isBusy() const {
    return busy_;
  }
  void setBusy(bool busy) {
    busy_ = busy;
  }

  // Own trace file, served by get_log.
  const QString& appLogPath() const {
    return appLogPath_;
  }

private slots:
  void onNewConnection();

private:
  bool busy_ = false;
  QString appLogPath_;
};

} // namespace lmc_agent

#endif // LMC_AGENT_SERVER_H
