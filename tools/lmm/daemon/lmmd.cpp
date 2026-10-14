// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Lmmd daemon implementation: peers, contacts polling and message routing.
#include "lmmd.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QRegularExpression>
#include <QTimer>

#include "core/settings.h"
#include "messaging/xmlmessage.h"

namespace {
const int kMaxEvents = 10000;
const int kMaxText = 8192;

QJsonObject contactJson(const User& u) {
  return QJsonObject{
    {"userId", u.id},
    {"name", u.name},
    {"address", u.address},
    {"status", u.status},
  };
}
} // namespace

Lmmd::Lmmd(const QString& socketName, QObject* parent)
    : QObject(parent), socketName_(socketName) {
  uptime_.start();
  auto* timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &Lmmd::pollContacts);
  timer->start(1000);
}

bool Lmmd::start(QString* error) {
  // Never steal the name from a live daemon: probe first, and only clear
  // a stale socket left by a crash.
  {
    QLocalSocket probe;
    probe.connectToServer(socketName_);
    if(probe.waitForConnected(500)) {
      probe.disconnectFromServer();
      if(error)
        *error = "name already taken by a live server: " + socketName_;
      return false;
    }
  }
  QLocalServer::removeServer(socketName_); // drop stale socket from a crash
  server_ = new QLocalServer(this);
  connect(server_, &QLocalServer::newConnection, this, [this]() {
    while(QLocalSocket* sock = server_->nextPendingConnection()) {
      auto* buf = new QByteArray(); // per-socket line buffer, freed on disconnect
      connect(sock, &QLocalSocket::readyRead, this, [this, sock, buf]() {
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
            // let the reply flush, then quit gracefully
            QTimer::singleShot(200, qApp, [this]() {
              stopAll();
              qApp->quit();
            });
          }
        }
      });
      connect(sock, &QLocalSocket::disconnected, this, [sock, buf]() {
        delete buf;
        sock->deleteLater();
      });
    }
  });
  if(!server_->listen(socketName_)) {
    if(error)
      *error = server_->errorString();
    return false;
  }
  return true;
}

void Lmmd::stopAll() {
  if(peers_.isEmpty())
    return; // idempotent: shutdown RPC and aboutToQuit both land here
  for(LmmPeer* p : peers_) {
    p->messaging->stop(); // sends Depart synchronously
    // Deferred delete: we may be inside this messaging's own signal stack
    // (shutdown RPC runs in readyRead). Depart already went out via stop().
    disconnect(p->messaging, nullptr, this, nullptr);
    p->messaging->deleteLater();
    delete p;
  }
  peers_.clear();
}

QJsonObject Lmmd::handleRequest(const QJsonObject& req) {
  const QString cmd = req.value("cmd").toString();
  QJsonValue id = req.value("id");
  if(id.isUndefined())
    id = QJsonValue(0); // missing id, same as the invalid-json path
  const QJsonObject params = req.value("params").toObject();
  QString error;
  QJsonObject result;
  bool ok = true;

  if(cmd == "status")
    result = cmdStatus();
  else if(cmd == "peer.add")
    result = cmdPeerAdd(params, &error);
  else if(cmd == "peer.list")
    result = cmdPeerList();
  else if(cmd == "peer.contacts")
    result = cmdPeerContacts(params, &error);
  else if(cmd == "peer.announce")
    result = cmdPeerAnnounce(params, &error);
  else if(cmd == "peer.remove")
    result = cmdPeerRemove(params, &error);
  else if(cmd == "msg.send")
    result = cmdMsgSend(params, &error);
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

bool Lmmd::validName(const QString& name) {
  static const QRegularExpression re("^[A-Za-z0-9_.-]{1,32}$");
  return re.match(name).hasMatch();
}

LmmPeer* Lmmd::findPeer(const QString& name) const {
  return peers_.value(name, nullptr);
}

void Lmmd::pushEvent(const QString& type, const QJsonObject& fields) {
  QJsonObject ev = fields;
  ev["seq"] = nextSeq_++;
  ev["t"] = QDateTime::currentMSecsSinceEpoch();
  ev["event"] = type;
  events_.append(ev);
  while(events_.size() > kMaxEvents)
    events_.removeFirst();
}

QJsonObject Lmmd::cmdStatus() {
  QJsonArray names;
  for(const QString& n : peers_.keys())
    names.append(n);
  return QJsonObject{
    {"socket", socketName_},
    {"peers", names},
    {"uptimeMs", uptime_.elapsed()},
    {"lmcVersion", IDA_VERSION},
  };
}

QJsonObject Lmmd::cmdPeerAdd(const QJsonObject& p, QString* error) {
  const QString name = p.value("name").toString();
  if(!validName(name)) {
    *error = "invalid name (1-32 of A-Za-z0-9_.-)";
    return {};
  }
  if(peers_.contains(name)) {
    *error = "peer already exists: " + name;
    return {};
  }
  // Identity: userId = MAC + $USER, so each peer needs its own $USER at init.
  // The name is captured into localUser at init; later USER changes do not
  // affect already created peers.
  qputenv("USER", name.toLatin1());
  {
    lmcSettings s;
    s.setValue(IDS_USERNAME, name);
    // All peers share one HOME/settings file: always write the status,
    // otherwise a peer added without one inherits a previous peer's leftover.
    const QString status = p.value("status").toString();
    s.setValue(IDS_STATUS, status.isEmpty() ? IDS_STATUS_VAL : status);
    s.sync();
  }
  auto* peer = new LmmPeer();
  peer->name = name;
  peer->messaging = new lmcMessaging();
  peer->messaging->setParent(this);
  XmlMessage init;
  peer->messaging->init(&init);
  // guard: MAC clash / same USER would produce a duplicate id
  for(LmmPeer* other : peers_) {
    if(other->messaging->localUser->id == peer->messaging->localUser->id) {
      // Synchronous delete is safe here: not started, nothing connected yet.
      delete peer->messaging;
      delete peer;
      *error = "userId collision (same MAC+USER as " + other->name + ")";
      return {};
    }
  }
  connect(peer->messaging, &lmcMessaging::messageReceived, this,
          [this, peer](MessageType type, QString* from, XmlMessage* msg) {
            onPeerMessage(peer, type, from, msg);
          });
  peer->messaging->start();
  peers_.insert(name, peer);
  pushEvent("peer.added", QJsonObject{{"peer", name}, {"userId", peer->messaging->localUser->id}});
  return QJsonObject{{"peer", name}, {"userId", peer->messaging->localUser->id}};
}

QJsonObject Lmmd::cmdPeerList() {
  QJsonArray arr;
  for(LmmPeer* peer : peers_) {
    arr.append(QJsonObject{
      {"peer", peer->name},
      {"userId", peer->messaging->localUser->id},
      {"displayName", peer->messaging->localUser->name},
      {"contacts", peer->messaging->userCount()},
    });
  }
  return QJsonObject{{"peers", arr}};
}

QJsonObject Lmmd::cmdPeerContacts(const QJsonObject& p, QString* error) {
  LmmPeer* peer = findPeer(p.value("peer").toString());
  if(!peer) {
    *error = "unknown peer";
    return {};
  }
  QJsonArray arr;
  for(const User& u : peer->messaging->userList)
    arr.append(contactJson(u));
  return QJsonObject{{"peer", peer->name}, {"contacts", arr}};
}

QJsonObject Lmmd::cmdPeerAnnounce(const QJsonObject& p, QString* error) {
  LmmPeer* peer = findPeer(p.value("peer").toString());
  if(!peer) {
    *error = "unknown peer";
    return {};
  }
  peer->messaging->update();
  return QJsonObject{{"peer", peer->name}, {"announced", true}};
}

QJsonObject Lmmd::cmdPeerRemove(const QJsonObject& p, QString* error) {
  LmmPeer* peer = findPeer(p.value("peer").toString());
  if(!peer) {
    *error = "unknown peer";
    return {};
  }
  peers_.remove(peer->name);
  peer->messaging->stop(); // sends Depart
  const QString userId = peer->messaging->localUser->id;
  // Deferred delete, see stopAll(); disconnect first so the pending
  // messageReceived lambda (capturing peer) can never fire afterwards.
  disconnect(peer->messaging, nullptr, this, nullptr);
  peer->messaging->deleteLater();
  pushEvent("peer.removed", QJsonObject{{"peer", peer->name}, {"userId", userId}});
  delete peer;
  return QJsonObject{{"removed", true}};
}

QString Lmmd::resolveUserId(LmmPeer* sender, const QString& to) const {
  if(LmmPeer* named = findPeer(to))
    return named->messaging->localUser->id;
  for(const User& u : sender->messaging->userList)
    if(u.id == to)
      return u.id;
  return {};
}

QJsonObject Lmmd::cmdMsgSend(const QJsonObject& p, QString* error) {
  LmmPeer* from = findPeer(p.value("from").toString());
  if(!from) {
    *error = "unknown sender peer";
    return {};
  }
  const QString text = p.value("text").toString();
  if(text.isEmpty()) {
    *error = "empty text";
    return {};
  }
  if(text.size() > kMaxText) {
    *error = "text too long (max 8192)";
    return {};
  }
  const QString userId = resolveUserId(from, p.value("to").toString());
  if(userId.isEmpty()) {
    *error = "unknown recipient (peer name or visible userId)";
    return {};
  }
  XmlMessage msg;
  msg.addHeader(XN_TIME, QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()));
  msg.addData(XN_MESSAGE, text);
  QString dest = userId;
  from->messaging->sendMessage(MT_Message, &dest, &msg);
  return QJsonObject{{"from", from->name}, {"to", userId}, {"sent", true}};
}

QJsonObject Lmmd::cmdEventsPoll(const QJsonObject& p) {
  const qint64 since = p.value("since").toVariant().toLongLong();
  const QString onlyPeer = p.value("peer").toString();
  QJsonArray arr;
  // next = highest seq RETURNED (not +1): events are appended with strictly
  // increasing seqs, so anything created after this scan has seq > next and
  // is picked up by the following poll. (+1 here would permanently skip an
  // event landing exactly in the inter-poll gap.)
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

void Lmmd::onPeerMessage(LmmPeer* peer, MessageType type, QString* from, XmlMessage* msg) {
  if(!from)
    return;
  if(type == MT_Failed) {
    // stack exhausted its retries (10s timeout, 1 retry): the message is dead,
    // clients should re-issue msg.send. Surfaced so tests can tell "failed"
    // apart from "not yet delivered".
    pushEvent("message.failed", QJsonObject{
                                  {"peer", peer->name},
                                  {"to", *from},
                                  {"text", msg ? msg->data(XN_MESSAGE) : QString()},
                                });
    return;
  }
  if(type != MT_Message || !msg)
    return;
  QString fromName;
  for(const User& u : peer->messaging->userList)
    if(u.id == *from) {
      fromName = u.name;
      break;
    }
  pushEvent("message.received", QJsonObject{
                                  {"peer", peer->name},
                                  {"from", *from},
                                  {"fromName", fromName},
                                  {"text", msg->data(XN_MESSAGE)},
                                });
}

void Lmmd::pollContacts() {
  for(LmmPeer* peer : peers_) {
    QSet<QString> now;
    QMap<QString, QString> names;
    for(const User& u : peer->messaging->userList) {
      now.insert(u.id);
      names[u.id] = u.name;
    }
    for(const QString& id : now - peer->contacts)
      pushEvent("contact.added", QJsonObject{{"peer", peer->name}, {"userId", id}, {"name", names[id]}});
    for(const QString& id : peer->contacts - now)
      pushEvent("contact.removed", QJsonObject{{"peer", peer->name}, {"userId", id}});
    peer->contacts = now;
  }
}
