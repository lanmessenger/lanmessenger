// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Supervisor: one external NDJSON face, one worker OS process per peer.
// External protocol is v1 wire-compatible (see PROTOCOL.md); workers are
// the same lmmd binary in hidden --worker mode on ephemeral loopback ports.
#ifndef LMM_SUPERVISOR_H
#define LMM_SUPERVISOR_H

#include <QElapsedTimer>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>

#include "core/definitions.h"

class QTcpServer;
class QTcpSocket;

struct SupervisorWorker {
  QString name;
  QString userId;
  QString displayName;
  QProcess* proc = nullptr;
  QTcpSocket* ctl = nullptr;
  QByteArray ctlBuf; // per-connection line buffer for blocking RPC
  qint64 nextId = 1; // control request ids
  qint64 cursor = 0; // worker events cursor
  bool dying = false; // expected death (peer.remove / shutdown)
};

// External daemon: spawns/reaps workers, routes peer.* to the owning
// worker, republishes worker events with a single global seq.
// Fail-fast: an unexpected worker death exits the daemon non-zero.
class Supervisor : public QObject {
  Q_OBJECT

 public:
  explicit Supervisor(quint16 port, const QString& homeDir, QObject* parent = nullptr);
  bool start(QString* error);
  // Graceful shutdown: Depart via workers, reap, then quit with code.
  void shutdown(int exitCode);

  QJsonObject handleRequest(const QJsonObject& req);
  int exitCode() const { return exitCode_; }

private:
  QJsonObject cmdStatus();
  QJsonObject cmdWorkerStatus(const QJsonObject& p, QString* error);
  QJsonObject cmdPeerAdd(const QJsonObject& p, QString* error);
  QJsonObject cmdPeerList(QString* error);
  QJsonObject cmdPeerContacts(const QJsonObject& p, QString* error);
  QJsonObject cmdPeerAnnounce(const QJsonObject& p, QString* error);
  QJsonObject cmdPeerReady(const QJsonObject& p, QString* error);
  QJsonObject cmdPeerRemove(const QJsonObject& p, QString* error);
  QJsonObject cmdMsgSend(const QJsonObject& p, QString* error);
  QJsonObject cmdFileSend(const QJsonObject& p, QString* error);
  QJsonObject cmdFileAccept(const QJsonObject& p, QString* error);
  QJsonObject cmdFileDecline(const QJsonObject& p, QString* error);
  QJsonObject cmdEventsPoll(const QJsonObject& p);

  void pushEvent(const QString& type, const QJsonObject& fields);
  // Blocking control call to one worker (ok=false + *error on failure).
  QJsonObject workerCall(SupervisorWorker* w, const QString& cmd,
                         const QJsonObject& params, QString* error);
  // Spawn + wait for readiness; on success fills userId/displayName.
  // Returns worker or nullptr (error set). Caller inserts + announces.
  SupervisorWorker* spawnWorker(const QString& name, const QString& status, QString* error);
  void dropWorker(SupervisorWorker* w); // reap + delete peer dir + free
  void pollWorkers(); // 500ms: drain worker events, republish
  void onWorkerFinished(int exitCode, QProcess::ExitStatus status);
  void onWorkerError(QProcess::ProcessError error);
  void fatal(const QString& msg, int code); // reap all, quit(code)
  SupervisorWorker* findWorker(const QString& name) const;

  quint16 port_;
  QString homeDir_;
  QTcpServer* server_ = nullptr;
  QMap<QString, SupervisorWorker*> workers_; // by handle
  QList<QJsonObject> events_; // ring, each has "seq"
  qint64 nextSeq_ = 1;
  QElapsedTimer uptime_;
  bool shuttingDown_ = false;
  bool polling_ = false; // reentrancy guard: blocking RPCs run nested loops
  int exitCode_ = 0;
};

#endif // LMM_SUPERVISOR_H
