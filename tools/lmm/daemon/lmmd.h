// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Lmmd daemon: N lmcMessaging peers behind a QTcpServer NDJSON API.
#ifndef LMMD_H
#define LMMD_H

#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>

#include "core/definitions.h"
#include "messaging/messaging.h"

struct LmmPeer {
  QString name; // handle used by clients ("Alice")
  lmcMessaging* messaging = nullptr;
  QSet<QString> contacts; // cached userIds, for contact.added/removed events
};

// lmmd: test daemon holding N lmcMessaging peers in one process.
// Control plane: newline-delimited JSON over TCP loopback (see PROTOCOL.md).
class Lmmd : public QObject {
  Q_OBJECT

 public:
  // port 0 = ephemeral (worker mode: the supervisor reads the actual port
  // back from the worker's stdout READY line).
  explicit Lmmd(quint16 port, QObject* parent = nullptr);
  // Worker mode: the daemon holds exactly one peer (auto-added by main).
  // Further peer.add is rejected so the supervisor's 1:1 mapping holds.
  void setSinglePeer(bool on) { singlePeer_ = on; }
  bool start(QString* error);
  void stopAll(); // Depart + stop every peer (graceful shutdown)

  QJsonObject handleRequest(const QJsonObject& req);
  // Actual bound port (differs from the constructor arg in ephemeral mode).
  quint16 serverPort() const;

private:
  // commands; each returns result object or sets error
  QJsonObject cmdStatus();
  QJsonObject cmdPeerAdd(const QJsonObject& p, QString* error);
  QJsonObject cmdPeerList();
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
  void onPeerMessage(LmmPeer* peer, MessageType type, QString* from, XmlMessage* msg);
  void onPeerFile(LmmPeer* peer, QString* from, XmlMessage* msg);
  void pollContacts(); // 1s timer: diff userList -> contact.added/removed

  LmmPeer* findPeer(const QString& name) const;
  QString resolveUserId(LmmPeer* sender, const QString& to) const;

  quint16 port_;
  class QTcpServer* server_ = nullptr;
  QMap<QString, LmmPeer*> peers_; // by handle
  bool singlePeer_ = false;
  // Orphan watchdog (worker mode only): the supervisor holds one persistent
  // control connection. If it had a client and has none for a while, the
  // supervisor is gone — quit instead of lingering with a stale socket.
  int clients_ = 0;
  bool everConnected_ = false;
  QElapsedTimer emptySince_;
  QList<QJsonObject> events_;     // ring, each has "seq"
  qint64 nextSeq_ = 1;
  QElapsedTimer uptime_;
  // Pending incoming file requests: fileId -> {peer, from, fileName, fileSize}.
  // Filled on MT_File FO_Request (mode receive), consumed by file.accept/decline.
  QMap<QString, QJsonObject> incomingFiles_;
};

#endif // LMMD_H
