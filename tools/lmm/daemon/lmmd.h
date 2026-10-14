// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Lmmd daemon: N lmcMessaging peers behind a QLocalServer NDJSON API.
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
// Control plane: newline-delimited JSON over QLocalServer (see PROTOCOL.md).
class Lmmd : public QObject {
  Q_OBJECT

public:
  explicit Lmmd(const QString& socketName, QObject* parent = nullptr);
  bool start(QString* error);
  void stopAll(); // Depart + stop every peer (graceful shutdown)

  QJsonObject handleRequest(const QJsonObject& req);

private:
  // commands; each returns result object or sets error
  QJsonObject cmdStatus();
  QJsonObject cmdPeerAdd(const QJsonObject& p, QString* error);
  QJsonObject cmdPeerList();
  QJsonObject cmdPeerContacts(const QJsonObject& p, QString* error);
  QJsonObject cmdPeerAnnounce(const QJsonObject& p, QString* error);
  QJsonObject cmdPeerRemove(const QJsonObject& p, QString* error);
  QJsonObject cmdMsgSend(const QJsonObject& p, QString* error);
  QJsonObject cmdEventsPoll(const QJsonObject& p);

  void pushEvent(const QString& type, const QJsonObject& fields);
  void onPeerMessage(LmmPeer* peer, MessageType type, QString* from, XmlMessage* msg);
  void pollContacts(); // 1s timer: diff userList -> contact.added/removed

  static bool validName(const QString& name);
  LmmPeer* findPeer(const QString& name) const;
  QString resolveUserId(LmmPeer* sender, const QString& to) const;

  QString socketName_;
  class QLocalServer* server_ = nullptr;
  QMap<QString, LmmPeer*> peers_; // by handle
  QList<QJsonObject> events_;     // ring, each has "seq"
  qint64 nextSeq_ = 1;
  QElapsedTimer uptime_;
};

#endif // LMMD_H
