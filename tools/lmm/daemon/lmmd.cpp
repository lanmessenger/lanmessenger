// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Lmmd daemon implementation: peers, contacts polling and message routing.
#include "lmmd.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include "core/settings.h"
#include "messaging/xmlmessage.h"
#include "rpc.h"

namespace {
const int kMaxEvents = 10000;
const int kMaxText = 8192;
// Upper bound for file.send payloads: comfortably above the 8MB regression
// file, far below memory pressure. The transfer itself streams in 64K
// chunks, so this only bounds the test fixture size.
const qint64 kMaxFileBytes = 256 * 1024 * 1024;

QJsonObject contactJson(const User& u) {
  return QJsonObject{
    {"userId", u.id},
    {"name", u.name},
    {"address", u.address},
    {"status", u.status},
  };
}
} // namespace

Lmmd::Lmmd(quint16 port, QObject* parent) : QObject(parent), port_(port) {
  uptime_.start();
  auto* timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &Lmmd::pollContacts);
  timer->start(1000);
}

bool Lmmd::start(QString* error) {
  // Ephemeral bind (:0) by construction: a fresh port cannot collide, so
  // there is no probe and no stale-artifact cleanup (the Unix-socket era's
  // removeServer dance died with the transport).
  server_ = new QTcpServer(this);
  connect(server_, &QTcpServer::newConnection, this, [this]() {
    while(QTcpSocket* sock = server_->nextPendingConnection()) {
      clients_++;
      auto* buf = new QByteArray(); // per-socket line buffer, freed on disconnect
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
            // let the reply flush, then quit gracefully
            QTimer::singleShot(200, qApp, [this]() {
              stopAll();
              qApp->quit();
            });
          }
        }
      });
      connect(sock, &QTcpSocket::disconnected, this, [this, sock, buf]() {
        clients_--;
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

quint16 Lmmd::serverPort() const {
  return server_ ? server_->serverPort() : 0;
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
  // Live settings path (same ctor as lmcSettings): XDG layout on Unix,
  // per-worker redirected layout on Windows. Tests locate the peer ini
  // through this instead of guessing OS layouts.
  const QString settingsFile =
    QSettings(QSettings::IniFormat, QSettings::UserScope, IDA_COMPANY, IDA_PRODUCT)
      .fileName();
  return QJsonObject{
    {"port", int(serverPort())},
    // Own pid for kill-target checks (crash suite): reported, not parsed
    // out of a platform process table.
    {"pid", int(QCoreApplication::applicationPid())},
    {"settingsFile", settingsFile},
    {"peers", names},
    {"uptimeMs", uptime_.elapsed()},
    {"lmcVersion", IDA_VERSION},
  };
}

QJsonObject Lmmd::cmdPeerAdd(const QJsonObject& p, QString* error) {
  const QString name = p.value("name").toString();
  if(!lmmRpc::validName(name)) {
    *error = "invalid name (1-32 of A-Za-z0-9_.-)";
    return {};
  }
  if(peers_.contains(name)) {
    *error = "peer already exists: " + name;
    return {};
  }
  if(singlePeer_ && !peers_.isEmpty()) {
    *error = "worker holds a single peer";
    return {};
  }
  // Identity: userId = MAC + $USER, so each peer needs its own $USER at init.
  // The name is captured into localUser at init; later USER changes do not
  // affect already created peers.
  qputenv("USER", name.toLatin1());
  {
    lmcSettings s;
    s.setValue(IDS_USERNAME, name);
    // Always write the status: a re-added peer (e.g. after an unclean
    // shutdown left its peer dir behind) must not inherit a stale one.
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

QJsonObject Lmmd::cmdPeerReady(const QJsonObject& p, QString* error) {
  LmmPeer* peer = findPeer(p.value("peer").toString());
  if(!peer) {
    *error = "unknown peer";
    return {};
  }
  // Unknown target polls as not ready, not an error.
  const QString target = p.value("user").toString();
  QString userId;
  for(const User& u : peer->messaging->userList) {
    if(u.name == target || u.id == target) {
      userId = u.id;
      break;
    }
  }
  const bool ready = !userId.isEmpty() && peer->messaging->isPeerReady(&userId);
  return QJsonObject{{"peer", peer->name}, {"user", userId}, {"ready", ready}};
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

QJsonObject Lmmd::cmdFileSend(const QJsonObject& p, QString* error) {
  LmmPeer* from = findPeer(p.value("from").toString());
  if(!from) {
    *error = "unknown sender peer";
    return {};
  }
  const QString path = p.value("path").toString();
  if(path.isEmpty()) {
    *error = "empty path";
    return {};
  }
  const QFileInfo info(path);
  if(!info.exists() || !info.isFile()) {
    *error = "path is not a readable file: " + path;
    return {};
  }
  if(info.size() <= 0) {
    *error = "empty file: " + path;
    return {};
  }
  if(info.size() > kMaxFileBytes) {
    *error = "file too large (max 256MB)";
    return {};
  }
  const QString userId = resolveUserId(from, p.value("to").toString());
  if(userId.isEmpty()) {
    *error = "unknown recipient (peer name or visible userId)";
    return {};
  }
  XmlMessage msg;
  msg.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
  msg.addData(XN_FILEOP, FileOpNames[FO_Request]);
  msg.addData(XN_FILEPATH, path);
  QString dest = userId;
  from->messaging->sendMessage(MT_File, &dest, &msg);
  return QJsonObject{
    {"from", from->name},
    {"to", userId},
    {"fileName", info.fileName()},
    {"fileSize", double(info.size())},
    {"sent", true},
  };
}

QJsonObject Lmmd::cmdFileAccept(const QJsonObject& p, QString* error) {
  const QString fileId = p.value("fileId").toString();
  if(fileId.isEmpty()) {
    *error = "empty fileId";
    return {};
  }
  if(!incomingFiles_.contains(fileId)) {
    *error = "unknown fileId (no pending request)";
    return {};
  }
  const QJsonObject pending = incomingFiles_.value(fileId);
  LmmPeer* peer = nullptr;
  const QString peerName = p.value("peer").toString();
  if(!peerName.isEmpty()) {
    peer = findPeer(peerName);
    if(!peer) {
      *error = "unknown peer";
      return {};
    }
    if(pending.value("peer").toString() != peer->name) {
      *error = "fileId belongs to another peer";
      return {};
    }
  } else {
    peer = findPeer(pending.value("peer").toString());
    if(!peer) {
      *error = "unknown peer";
      return {};
    }
  }
  QString senderId = pending.value("from").toString();
  XmlMessage msg;
  msg.addData(XN_MODE, FileModeNames[FM_Receive]);
  msg.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
  msg.addData(XN_FILEOP, FileOpNames[FO_Accept]);
  msg.addData(XN_FILEID, fileId);
  // Mirror the GUI accept (messagelog.cpp): the stack overwrites FILEPATH
  // with the storage path, but FILESIZE is never set downstream — it must
  // ride in the message or both ends run with size 0 and no byte flows.
  msg.addData(XN_FILENAME, pending.value("fileName").toString());
  msg.addData(XN_FILESIZE, QString::number(pending.value("fileSize").toDouble(), 'f', 0));
  msg.addData(XN_FILEPATH, pending.value("filePath").toString());
  peer->messaging->sendMessage(MT_File, &senderId, &msg);
  incomingFiles_.remove(fileId);
  return QJsonObject{{"peer", peer->name}, {"fileId", fileId}, {"accepted", true}};
}

QJsonObject Lmmd::cmdFileDecline(const QJsonObject& p, QString* error) {
  const QString fileId = p.value("fileId").toString();
  if(fileId.isEmpty()) {
    *error = "empty fileId";
    return {};
  }
  if(!incomingFiles_.contains(fileId)) {
    *error = "unknown fileId (no pending request)";
    return {};
  }
  const QJsonObject pending = incomingFiles_.value(fileId);
  LmmPeer* peer = findPeer(pending.value("peer").toString());
  if(!peer) {
    *error = "unknown peer";
    return {};
  }
  QString senderId = pending.value("from").toString();
  XmlMessage msg;
  msg.addData(XN_MODE, FileModeNames[FM_Receive]);
  msg.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
  msg.addData(XN_FILEOP, FileOpNames[FO_Decline]);
  msg.addData(XN_FILEID, fileId);
  peer->messaging->sendMessage(MT_File, &senderId, &msg);
  incomingFiles_.remove(fileId);
  return QJsonObject{{"peer", peer->name}, {"fileId", fileId}, {"declined", true}};
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
  if(type != MT_Message || !msg) {
    if((type == MT_File || type == MT_Avatar) && msg)
      onPeerFile(peer, from, msg);
    return;
  }
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

void Lmmd::onPeerFile(LmmPeer* peer, QString* from, XmlMessage* msg) {
  // Only real file payloads: avatars/folders take their own paths and must
  // not pollute the file suite (avatars auto-accept inside the stack).
  if(msg->data(XN_FILETYPE) != FileTypeNames[FT_Normal])
    return;
  const int fileOp = Helper::indexOf(FileOpNames, FO_Max, msg->data(XN_FILEOP));
  const int fileMode = Helper::indexOf(FileModeNames, FM_Max, msg->data(XN_MODE));
  const QString fileId = msg->data(XN_FILEID);
  if(fileId.isEmpty())
    return;
  const QString modeName =
      (fileMode > FM_Blank && fileMode < FM_Max) ? FileModeNames[fileMode] : QString();
  const QString opName = (fileOp > FO_Blank && fileOp < FO_Max) ? FileOpNames[fileOp] : QString();

  switch(fileOp) {
  case FO_Request:
    // Incoming only: the sender's own local echo (mode send) is UI noise.
    if(fileMode != FM_Receive)
      return;
    {
      QString fromName;
      for(const User& u : peer->messaging->userList)
        if(u.id == *from) {
          fromName = u.name;
          break;
        }
      incomingFiles_[fileId] = QJsonObject{
        {"peer", peer->name},
        {"from", *from},
        {"fileName", msg->data(XN_FILENAME)},
        {"fileSize", double(msg->data(XN_FILESIZE).toLongLong())},
        {"filePath", msg->data(XN_FILEPATH)},
      };
      pushEvent("file.request", QJsonObject{
                                  {"peer", peer->name},
                                  {"from", *from},
                                  {"fromName", fromName},
                                  {"fileId", fileId},
                                  {"fileName", msg->data(XN_FILENAME)},
                                  {"fileSize", double(msg->data(XN_FILESIZE).toLongLong())},
                                });
    }
    return;
  case FO_Accept:
  case FO_Decline:
  case FO_Cancel:
    pushEvent(fileOp == FO_Accept ? "file.accepted"
              : fileOp == FO_Decline             ? "file.declined"
                                                 : "file.cancelled",
              QJsonObject{
                {"peer", peer->name},
                {"from", *from},
                {"fileId", fileId},
                {"mode", modeName},
                {"op", opName},
              });
    return;
  case FO_Complete:
  case FO_Error:
  case FO_Abort: {
    // path rides in XN_FILEPATH for complete/error (tcpnetwork::update);
    // derive name/size from disk when present so tests need no guessing.
    const QString path = msg->data(XN_FILEPATH);
    QString fileName = msg->data(XN_FILENAME);
    double fileSize = double(msg->data(XN_FILESIZE).toLongLong());
    if(!path.isEmpty()) {
      const QFileInfo info(path);
      if(fileName.isEmpty())
        fileName = info.fileName();
      if(fileSize <= 0 && info.exists())
        fileSize = double(info.size());
    }
    // A completed receive is no longer pending (accept already removed it,
    // but keep this idempotent for races).
    if(fileOp == FO_Complete || fileOp == FO_Error || fileOp == FO_Abort)
      incomingFiles_.remove(fileId);
    pushEvent(fileOp == FO_Complete ? "file.completed" : "file.failed",
              QJsonObject{
                {"peer", peer->name},
                {"from", *from},
                {"fileId", fileId},
                {"mode", modeName},
                {"op", opName},
                {"path", path},
                {"fileName", fileName},
                {"fileSize", fileSize},
              });
    return;
  }
  default:
    return; // progress/init/next: too noisy for the event log
  }
}

void Lmmd::pollContacts() {
  if(singlePeer_) {
    // Orphan watchdog: normal operation holds one persistent supervisor
    // connection. Had a client, now none for a while => supervisor is gone.
    // (Normal shutdown arrives as a "shutdown" RPC first and quits above.)
    if(clients_ > 0) {
      everConnected_ = true;
      emptySince_.invalidate();
    } else if(everConnected_) {
      if(!emptySince_.isValid())
        emptySince_.start();
      else if(emptySince_.elapsed() > 5000) {
        qWarning("lmmd worker: supervisor gone, quitting");
        qApp->quit();
        return;
      }
    }
  }
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
