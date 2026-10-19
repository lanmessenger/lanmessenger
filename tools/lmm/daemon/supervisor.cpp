// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Supervisor implementation: worker lifecycle, routing, event merge.
#include "supervisor.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include "rpc.h"

namespace {
const int kMaxEvents = 10000;
const int kRpcTimeoutMs = 10000;
const int kSpawnTimeoutMs = 15000;
const int kShutdownWaitMs = 5000;

// Worker rendezvous: the worker binds 127.0.0.1:0 and prints
// "READY <port>" on stdout (its logs go to stderr). No port-picking
// race by construction: the supervisor dials the announced port.
quint16 readWorkerPort(QProcess* proc, QString* error) {
  QByteArray out;
  QElapsedTimer timer;
  timer.start();
  while(timer.elapsed() < kSpawnTimeoutMs) {
    if(proc->state() == QProcess::NotRunning) {
      if(error)
        *error = "worker exited before READY (crashed on startup?)";
      return 0;
    }
    proc->waitForReadyRead(250);
    out += proc->readAllStandardOutput();
    const int nl = out.indexOf('\n');
    if(nl < 0)
      continue;
    const QByteArray line = out.left(nl).trimmed();
    if(line.startsWith("READY ")) {
      bool ok = false;
      const quint16 port = line.mid(6).trimmed().toUShort(&ok);
      if(ok && port != 0)
        return port;
    }
    if(error)
      *error = "worker printed garbage instead of READY: " + QString::fromUtf8(line);
    return 0;
  }
  if(error)
    *error = "worker printed no READY in time (crashed on startup?)";
  return 0;
}
} // namespace

Supervisor::Supervisor(quint16 port, const QString& homeDir, QObject* parent)
    : QObject(parent), port_(port), homeDir_(homeDir) {
  uptime_.start();
  auto* timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &Supervisor::pollWorkers);
  timer->start(500);
}

bool Supervisor::start(QString* error) {
  // Never steal the port from a live daemon. No stale-artifact cleanup:
  // a TCP port frees itself when the process dies (unlike socket files).
  {
    QTcpSocket probe;
    probe.connectToHost(QHostAddress::LocalHost, port_);
    if(probe.waitForConnected(500)) {
      probe.disconnectFromHost();
      if(error)
        *error = QString("port already taken by a live server: %1").arg(port_);
      return false;
    }
  }
  server_ = new QTcpServer(this);
  connect(server_, &QTcpServer::newConnection, this, [this]() {
    while(QTcpSocket* sock = server_->nextPendingConnection()) {
      auto* buf = new QByteArray();
      connect(sock, &QTcpSocket::readyRead, this, [this, sock, buf]() {
        buf->append(sock->readAll());
        int nl = -1;
        while((nl = buf->indexOf('\n')) >= 0) {
          QByteArray line = buf->left(nl).trimmed();
          buf->remove(0, nl + 1);
          if(line.isEmpty())
            continue;
          QJsonParseError perr;
          QJsonDocument doc = QJsonDocument::fromJson(line, &perr);
          QJsonObject resp;
          if(perr.error != QJsonParseError::NoError || !doc.isObject())
            resp = QJsonObject{{"id", 0}, {"ok", false}, {"error", "invalid json"}};
          else
            resp = handleRequest(doc.object());
          sock->write(QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n");
          sock->flush();
          if(doc.isObject() && doc.object().value("cmd").toString() == "shutdown") {
            // Reply flushed; graceful stop lands in shutdown(), then quit.
            QTimer::singleShot(200, qApp, [this]() { shutdown(0); });
          }
        }
      });
      connect(sock, &QTcpSocket::disconnected, this, [sock, buf]() {
        delete buf;
        sock->deleteLater();
      });
    }
  });
  if(!server_->listen(QHostAddress::LocalHost, port_)) {
    if(error)
      *error = server_->errorString();
    return false;
  }
  return true;
}

void Supervisor::shutdown(int exitCode) {
  if(shuttingDown_) {
    exitCode_ = exitCode;
    return; // idempotent: shutdown RPC and aboutToQuit both land here
  }
  shuttingDown_ = true;
  exitCode_ = exitCode;
  // Ask all workers to Depart in parallel, then reap with a deadline.
  for(SupervisorWorker* w : workers_) {
    w->dying = true;
    if(w->ctl && w->ctl->state() == QTcpSocket::ConnectedState) {
      QString ignored;
      workerCall(w, "shutdown", QJsonObject(), &ignored);
    }
  }
  for(SupervisorWorker* w : workers_) {
    if(w->proc->state() != QProcess::NotRunning)
      w->proc->waitForFinished(kShutdownWaitMs);
    if(w->proc->state() != QProcess::NotRunning) {
      qWarning("supervisor: worker %s ignored shutdown, killing", qPrintable(w->name));
      w->proc->kill();
      w->proc->waitForFinished(2000);
    }
  }
  qApp->quit();
}

QJsonObject Supervisor::handleRequest(const QJsonObject& req) {
  const QString cmd = req.value("cmd").toString();
  QJsonValue id = req.value("id");
  if(id.isUndefined())
    id = QJsonValue(0);
  const QJsonObject params = req.value("params").toObject();
  QString error;
  QJsonObject result;
  bool ok = true;

  if(shuttingDown_) {
    ok = false;
    error = "shutting down";
  } else if(cmd == "status")
    result = cmdStatus();
  else if(cmd == "worker.status")
    result = cmdWorkerStatus(params, &error);
  else if(cmd == "peer.add")
    result = cmdPeerAdd(params, &error);
  else if(cmd == "peer.list")
    result = cmdPeerList(&error);
  else if(cmd == "peer.contacts")
    result = cmdPeerContacts(params, &error);
  else if(cmd == "peer.announce")
    result = cmdPeerAnnounce(params, &error);
  else if(cmd == "peer.ready")
    result = cmdPeerReady(params, &error);
  else if(cmd == "peer.remove")
    result = cmdPeerRemove(params, &error);
  else if(cmd == "msg.send")
    result = cmdMsgSend(params, &error);
  else if(cmd == "file.send")
    result = cmdFileSend(params, &error);
  else if(cmd == "file.accept")
    result = cmdFileAccept(params, &error);
  else if(cmd == "file.decline")
    result = cmdFileDecline(params, &error);
  else if(cmd == "events.poll")
    result = cmdEventsPoll(params);
  else if(cmd == "shutdown")
    result = QJsonObject{{"stopping", true}};
  else {
    ok = false;
    error = "unknown cmd: " + cmd;
  }
  if(!error.isEmpty())
    ok = false;

  QJsonObject resp{{"id", id}, {"ok", ok}};
  if(ok)
    resp["result"] = result;
  else
    resp["error"] = error;
  return resp;
}

void Supervisor::pushEvent(const QString& type, const QJsonObject& fields) {
  QJsonObject ev = fields;
  ev["seq"] = nextSeq_++;
  ev["t"] = QDateTime::currentMSecsSinceEpoch();
  ev["event"] = type;
  events_.append(ev);
  while(events_.size() > kMaxEvents)
    events_.removeFirst();
}

SupervisorWorker* Supervisor::findWorker(const QString& name) const {
  return workers_.value(name, nullptr);
}

QJsonObject Supervisor::workerCall(SupervisorWorker* w, const QString& cmd,
                                   const QJsonObject& params, QString* error) {
  if(!w->ctl || w->ctl->state() != QTcpSocket::ConnectedState) {
    if(error)
      *error = "worker control down: " + w->name;
    return {};
  }
  return lmmRpc::exchange(w->ctl, &w->ctlBuf, lmmRpc::makeRequest(w->nextId++, cmd, params),
                          kRpcTimeoutMs, error);
}

SupervisorWorker* Supervisor::spawnWorker(const QString& name, const QString& status,
                                          QString* error) {
  const QString peerDir = homeDir_ + "/peers/" + name;
  if(!QDir().mkpath(peerDir)) {
    if(error)
      *error = "cannot create peer dir: " + peerDir;
    return nullptr;
  }
  QStringList args{"--worker", "--peer", name, "--home", peerDir};
  if(!status.isEmpty())
    args << "--status" << status;

  auto* proc = new QProcess(this);
  proc->setProgram(QCoreApplication::applicationFilePath());
  proc->setArguments(args);
  // Worker logs to our stderr with a prefix: the only window into crashes.
  connect(proc, &QProcess::readyReadStandardError, this, [proc, name]() {
    for(const QByteArray& line : proc->readAllStandardError().split('\n')) {
      if(!line.trimmed().isEmpty())
        qWarning("[%s] %s", qPrintable(name), line.constData());
    }
  });

  auto* w = new SupervisorWorker();
  w->name = name;
  w->proc = proc;
  // finished/error handlers need the worker; resolve by sender().
  connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &Supervisor::onWorkerFinished);
  connect(proc, &QProcess::errorOccurred, this, &Supervisor::onWorkerError);

  proc->start();
  if(!proc->waitForStarted(kSpawnTimeoutMs)) {
    if(error)
      *error = "worker failed to start: " + proc->errorString();
    proc->deleteLater();
    delete w;
    return nullptr;
  }
  const quint16 workerPort = readWorkerPort(proc, error);
  if(workerPort == 0) {
    if(proc->state() != QProcess::NotRunning) {
      proc->kill();
      proc->waitForFinished(2000);
    }
    proc->deleteLater();
    delete w;
    return nullptr;
  }
  auto* ctl = new QTcpSocket(this);
  if(!lmmRpc::connectRetry(ctl, QHostAddress::LocalHost, workerPort, kSpawnTimeoutMs)) {
    if(error)
      *error = "worker control unreachable (crashed on startup?)";
    if(proc->state() != QProcess::NotRunning) {
      proc->kill();
      proc->waitForFinished(2000);
    }
    proc->deleteLater();
    delete ctl;
    delete w;
    return nullptr;
  }
  w->ctl = ctl;
  QString callError;
  QJsonObject list = workerCall(w, "peer.list", QJsonObject(), &callError);
  if(!callError.isEmpty()) {
    if(error)
      *error = "worker peer.list failed: " + callError;
    w->dying = true;
    if(proc->state() != QProcess::NotRunning) {
      proc->kill();
      proc->waitForFinished(2000);
    }
    proc->deleteLater();
    delete w->ctl;
    delete w;
    return nullptr;
  }
  QJsonArray peers = list.value("peers").toArray();
  if(peers.size() != 1) {
    if(error)
      *error = "worker holds no peer (startup race?)";
    w->dying = true;
    if(proc->state() != QProcess::NotRunning) {
      proc->kill();
      proc->waitForFinished(2000);
    }
    proc->deleteLater();
    delete w->ctl;
    delete w;
    return nullptr;
  }
  QJsonObject self = peers.first().toObject();
  w->userId = self.value("userId").toString();
  w->displayName = self.value("displayName").toString();
  // Initial drain: drop everything queued before we connected
  // (the worker's own peer.added belongs to it, not to us).
  QJsonObject drain = workerCall(w, "events.poll", QJsonObject(), &callError);
  if(callError.isEmpty())
    w->cursor = (qint64)drain.value("next").toDouble();
  return w;
}

void Supervisor::dropWorker(SupervisorWorker* w) {
  workers_.remove(w->name);
  // Reap first so the Depart is really on the wire before we return.
  if(w->proc->state() != QProcess::NotRunning) {
    w->proc->waitForFinished(kShutdownWaitMs);
    if(w->proc->state() != QProcess::NotRunning) {
      qWarning("supervisor: worker %s stuck, killing", qPrintable(w->name));
      w->proc->kill();
      w->proc->waitForFinished(2000);
    }
  }
  QDir(homeDir_ + "/peers/" + w->name).removeRecursively();
  w->proc->deleteLater();
  // ctl socket is parented to us; the struct is plain data, freed here.
  // dropWorker only runs from request handlers, never from the socket's
  // own signals, so direct delete is safe.
  delete w->ctl;
  const QString userId = w->userId;
  const QString name = w->name;
  delete w;
  pushEvent("peer.removed", QJsonObject{{"peer", name}, {"userId", userId}});
}

void Supervisor::pollWorkers() {
  if(shuttingDown_ || polling_)
    return; // never reenter: blocking RPCs below pump nested event loops
  polling_ = true;
  // Copy: nested event loops (blocking RPCs below) can deliver external
  // peer.add/peer.remove meanwhile, mutating the map mid-iteration.
  const QList<SupervisorWorker*> snapshot = workers_.values();
  for(SupervisorWorker* w : snapshot) {
    if(!workers_.contains(w->name))
      continue; // dropped by a nested request
    if(w->dying)
      continue;
    if(!w->ctl || w->ctl->state() != QTcpSocket::ConnectedState)
      continue;
    QString error;
    QJsonObject res = workerCall(w, "events.poll",
                                QJsonObject{{"since", double(w->cursor)}}, &error);
    if(!error.isEmpty())
      continue; // transient; unexpected death is handled via finished()
    w->cursor = (qint64)res.value("next").toDouble(w->cursor);
    for(const QJsonValue& v : res.value("events").toArray()) {
      QJsonObject ev = v.toObject();
      const QString type = ev.value("event").toString();
      if(type == "peer.added" || type == "peer.removed")
        continue; // owned by the supervisor, never republished
      ev.remove("seq");
      ev.remove("t");
      pushEvent(type, ev);
    }
  }
  polling_ = false;
}

void Supervisor::onWorkerFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  auto* proc = qobject_cast<QProcess*>(sender());
  SupervisorWorker* hit = nullptr;
  for(SupervisorWorker* w : workers_) {
    if(w->proc == proc) {
      hit = w;
      break;
    }
  }
  if(!hit)
    return; // already dropped (remove/shutdown path reaped it)
  if(shuttingDown_ || hit->dying)
    return; // expected death; reaper is dropWorker/shutdown
  fatal(QString("worker %1 died unexpectedly (exit=%2, %3)")
            .arg(hit->name)
            .arg(exitCode)
            .arg(exitStatus == QProcess::CrashExit ? "crashed" : "exited"),
        1);
}

void Supervisor::onWorkerError(QProcess::ProcessError error) {
  if(error != QProcess::FailedToStart)
    return;
  // Spawn-time failures are reported synchronously by spawnWorker via
  // waitForStarted(); a mapped worker can no longer fail to start.
  // Still, never let one slip through silently.
  auto* proc = qobject_cast<QProcess*>(sender());
  for(SupervisorWorker* w : workers_) {
    if(w->proc == proc && !w->dying) {
      fatal(QString("worker %1 failed to start: %2").arg(w->name, proc->errorString()), 1);
      return;
    }
  }
}

void Supervisor::fatal(const QString& msg, int code) {
  if(shuttingDown_) {
    exitCode_ = code;
    return;
  }
  qCritical("supervisor: %s", qPrintable(msg));
  shutdown(code);
}

QJsonObject Supervisor::cmdStatus() {
  QJsonArray names;
  for(const QString& n : workers_.keys())
    names.append(n);
  return QJsonObject{
    {"port", int(port_)},
    {"peers", names},
    {"uptimeMs", uptime_.elapsed()},
    {"lmcVersion", IDA_VERSION},
  };
}

QJsonObject Supervisor::cmdWorkerStatus(const QJsonObject& p, QString* error) {
  // Introspection for tests (settings-file location differs per OS):
  // returns the worker's own status dict verbatim.
  SupervisorWorker* w = findWorker(p.value("peer").toString());
  if(!w) {
    *error = "unknown peer";
    return {};
  }
  return workerCall(w, "status", QJsonObject(), error);
}

QJsonObject Supervisor::cmdPeerAdd(const QJsonObject& p, QString* error) {
  const QString name = p.value("name").toString();
  if(!lmmRpc::validName(name)) {
    *error = "invalid name (1-32 of A-Za-z0-9_.-)";
    return {};
  }
  if(workers_.contains(name)) {
    *error = "peer already exists: " + name;
    return {};
  }
  const QString status = p.value("status").toString();
  SupervisorWorker* w = spawnWorker(name, status, error);
  if(!w)
    return {};
  // Defensive: same box + same handle in another daemon => same userId,
  // and the pair would self-filter each other. Refuse loudly.
  for(SupervisorWorker* other : workers_) {
    if(other->userId == w->userId) {
      w->dying = true;
      QString ignored;
      workerCall(w, "shutdown", QJsonObject(), &ignored);
      if(w->proc->state() != QProcess::NotRunning) {
        w->proc->waitForFinished(kShutdownWaitMs);
        if(w->proc->state() != QProcess::NotRunning)
          w->proc->kill();
      }
      w->proc->deleteLater();
      delete w->ctl;
      QDir(homeDir_ + "/peers/" + name).removeRecursively();
      delete w;
      *error = "userId collision (same MAC+USER as " + other->name + ")";
      return {};
    }
  }
  workers_.insert(name, w);
  pushEvent("peer.added", QJsonObject{{"peer", name}, {"userId", w->userId}});
  return QJsonObject{{"peer", name}, {"userId", w->userId}};
}

QJsonObject Supervisor::cmdPeerList(QString* error) {
  QJsonArray arr;
  for(SupervisorWorker* w : workers_) {
    QString callError;
    QJsonObject res = workerCall(w, "peer.list", QJsonObject(), &callError);
    if(!callError.isEmpty()) {
      if(error)
        *error = "worker down: " + w->name + " (" + callError + ")";
      return {};
    }
    for(const QJsonValue& v : res.value("peers").toArray())
      arr.append(v);
  }
  return QJsonObject{{"peers", arr}};
}

QJsonObject Supervisor::cmdPeerContacts(const QJsonObject& p, QString* error) {
  SupervisorWorker* w = findWorker(p.value("peer").toString());
  if(!w) {
    *error = "unknown peer";
    return {};
  }
  return workerCall(w, "peer.contacts", QJsonObject{{"peer", w->name}}, error);
}

QJsonObject Supervisor::cmdPeerAnnounce(const QJsonObject& p, QString* error) {
  SupervisorWorker* w = findWorker(p.value("peer").toString());
  if(!w) {
    *error = "unknown peer";
    return {};
  }
  return workerCall(w, "peer.announce", QJsonObject{{"peer", w->name}}, error);
}

QJsonObject Supervisor::cmdPeerReady(const QJsonObject& p, QString* error) {
  SupervisorWorker* w = findWorker(p.value("peer").toString());
  if(!w) {
    *error = "unknown peer";
    return {};
  }
  QString user = p.value("user").toString();
  if(SupervisorWorker* named = findWorker(user))
    user = named->userId;
  return workerCall(w, "peer.ready", QJsonObject{{"peer", w->name}, {"user", user}}, error);
}

QJsonObject Supervisor::cmdPeerRemove(const QJsonObject& p, QString* error) {
  SupervisorWorker* w = findWorker(p.value("peer").toString());
  if(!w) {
    *error = "unknown peer";
    return {};
  }
  workers_.remove(w->name);
  w->dying = true; // death from here on is expected
  QString ignored;
  workerCall(w, "shutdown", QJsonObject(), &ignored); // Depart first
  dropWorker(w); // reaps, deletes peer dir, emits peer.removed
  return QJsonObject{{"removed", true}};
}

QJsonObject Supervisor::cmdMsgSend(const QJsonObject& p, QString* error) {
  SupervisorWorker* from = findWorker(p.value("from").toString());
  if(!from) {
    *error = "unknown sender peer";
    return {};
  }
  const QString text = p.value("text").toString();
  if(text.isEmpty()) {
    *error = "empty text";
    return {};
  }
  if(text.size() > 8192) {
    *error = "text too long (max 8192)";
    return {};
  }
  // Peer-handle resolution is supervisor-wide (it knows every userId);
  // raw userIds pass through to the sender's own visibility check.
  QString to = p.value("to").toString();
  if(SupervisorWorker* named = findWorker(to))
    to = named->userId;
  QJsonObject fwd{{"from", from->name}, {"to", to}, {"text", text}};
  return workerCall(from, "msg.send", fwd, error);
}

QJsonObject Supervisor::cmdFileSend(const QJsonObject& p, QString* error) {
  SupervisorWorker* from = findWorker(p.value("from").toString());
  if(!from) {
    *error = "unknown sender peer";
    return {};
  }
  const QString path = p.value("path").toString();
  if(path.isEmpty()) {
    *error = "empty path";
    return {};
  }
  QString to = p.value("to").toString();
  if(SupervisorWorker* named = findWorker(to))
    to = named->userId;
  QJsonObject fwd{{"from", from->name}, {"to", to}, {"path", path}};
  return workerCall(from, "file.send", fwd, error);
}

QJsonObject Supervisor::cmdFileAccept(const QJsonObject& p, QString* error) {
  SupervisorWorker* w = findWorker(p.value("peer").toString());
  if(!w) {
    *error = "unknown peer";
    return {};
  }
  const QString fileId = p.value("fileId").toString();
  if(fileId.isEmpty()) {
    *error = "empty fileId";
    return {};
  }
  return workerCall(w, "file.accept", QJsonObject{{"peer", w->name}, {"fileId", fileId}}, error);
}

QJsonObject Supervisor::cmdFileDecline(const QJsonObject& p, QString* error) {
  SupervisorWorker* w = findWorker(p.value("peer").toString());
  if(!w) {
    *error = "unknown peer";
    return {};
  }
  const QString fileId = p.value("fileId").toString();
  if(fileId.isEmpty()) {
    *error = "empty fileId";
    return {};
  }
  return workerCall(w, "file.decline", QJsonObject{{"peer", w->name}, {"fileId", fileId}}, error);
}

QJsonObject Supervisor::cmdEventsPoll(const QJsonObject& p) {
  const qint64 since = p.value("since").toVariant().toLongLong();
  const QString onlyPeer = p.value("peer").toString();
  QJsonArray arr;
  qint64 next = since;
  for(const QJsonObject& ev : events_) {
    const qint64 seq = ev.value("seq").toVariant().toLongLong();
    if(seq <= since)
      continue;
    if(!onlyPeer.isEmpty() && ev.value("peer").toString() != onlyPeer)
      continue;
    arr.append(ev);
    if(seq > next)
      next = seq;
  }
  return QJsonObject{{"events", arr}, {"next", double(next)}};
}
