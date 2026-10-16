// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Shared NDJSON helpers for the lmmd supervisor <-> worker control plane.
// Header-only: used by the worker engine (lmmd.*) and the supervisor.
#ifndef LMM_RPC_H
#define LMM_RPC_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QString>
#include <QTcpSocket>
#include <QThread>

namespace lmmRpc {

// Peer handle rule, shared by supervisor and worker.
inline bool validName(const QString& name) {
  static const QRegularExpression re("^[A-Za-z0-9_.-]{1,32}$");
  return re.match(name).hasMatch();
}

inline QJsonObject makeRequest(const QJsonValue& id, const QString& cmd,
                               const QJsonObject& params = QJsonObject()) {
  return QJsonObject{{"id", id}, {"cmd", cmd}, {"params", params}};
}

// Blocking request/response over a connected QTcpSocket.
// *buf is the per-socket line buffer, kept across calls.
// On success returns the "result" object; on failure returns {} and
// sets *error. The caller owns timeout budget (single call, no retries).
inline QJsonObject exchange(QTcpSocket* sock, QByteArray* buf, const QJsonObject& req,
                            int timeoutMs, QString* error) {
  const QJsonValue id = req.value("id");
  sock->write(QJsonDocument(req).toJson(QJsonDocument::Compact) + "\n");
  if(!sock->waitForBytesWritten(timeoutMs)) {
    if(error)
      *error = "control write failed: " + sock->errorString();
    return {};
  }
  QElapsedTimer timer;
  timer.start();
  while(true) {
    int nl = buf->indexOf('\n');
    if(nl < 0) {
      int left = timeoutMs - (int)timer.elapsed();
      if(left <= 0 || !sock->waitForReadyRead(left)) {
        if(error)
          *error = "control read timeout: " + sock->errorString();
        return {};
      }
      buf->append(sock->readAll());
      continue;
    }
    QByteArray line = buf->left(nl).trimmed();
    buf->remove(0, nl + 1);
    if(line.isEmpty())
      continue;
    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(line, &perr);
    if(perr.error != QJsonParseError::NoError || !doc.isObject()) {
      if(error)
        *error = "control garbled response";
      return {};
    }
    QJsonObject resp = doc.object();
    if(resp.value("id") != id)
      continue; // stray line (e.g. id 0); keep waiting within budget
    if(!resp.value("ok").toBool()) {
      if(error)
        *error = resp.value("error").toString("control error");
      return {};
    }
    return resp.value("result").toObject();
  }
}

// Connect with retries; short sleeps, no event-loop reentrancy.
inline bool connectRetry(QTcpSocket* sock, const QHostAddress& host, quint16 port,
                         int timeoutMs) {
  QElapsedTimer timer;
  timer.start();
  while(true) {
    sock->connectToHost(host, port);
    if(sock->waitForConnected(300))
      return true;
    if(timer.elapsed() > timeoutMs)
      return false;
    QThread::msleep(100);
  }
}

} // namespace lmmRpc

#endif // LMM_RPC_H
