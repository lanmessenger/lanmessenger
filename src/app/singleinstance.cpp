// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "singleinstance.h"

#include "core/lmccategories.h"
#include <QDeadlineTimer>
#include <QDir>
#include <QCryptographicHash>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QThread>

#if defined(Q_OS_WIN)
#include <QtCore/qt_windows.h>
#else
#include <unistd.h>
#endif

namespace {

bool inFlatpakSandbox() {
  // No caching: the test binary toggles FLATPAK_ID per test.
  return QFile::exists(QStringLiteral("/.flatpak-info")) ||
         qEnvironmentVariableIsSet("FLATPAK_ID");
}

// Sandbox-shared, persistent IPC dir. Each `flatpak run` gets a private
// /tmp, so tempPath() cannot host the lock/socket; $HOME (and the XDG
// dirs under it) maps to ~/.var/app/<id> shared by all instances.
// GenericCacheLocation carries no app-name suffix, so it is valid here
// even though SingleInstance is built before setApplicationName().
QString flatpakIpcDir() {
  QString base = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
  if(base.isEmpty())
    base = QDir(QDir::homePath()).absoluteFilePath(QStringLiteral(".cache"));
  return QDir(base).absoluteFilePath(QStringLiteral("lmc/ipc"));
}

constexpr char kAckOpcode = 'A';
constexpr char kRaiseOpcode = 'R';
constexpr char kTermOpcode = 'T';
constexpr int kOpcodeTimeoutMs = 1000;

// The IPC alphabet is two commands ("raise"/"term", see remoteCommandFor);
// anything else maps to raise.
char opcodeFor(const QString& message) {
  return message.compare(QStringLiteral("term"), Qt::CaseInsensitive) == 0 ? kTermOpcode
                                                                           : kRaiseOpcode;
}

QString messageForOpcode(char opcode) {
  return opcode == kTermOpcode ? QStringLiteral("term") : QStringLiteral("raise");
}

#if defined(Q_OS_WIN)
// Per-session isolation. ProcessIdToSessionId is available since Windows XP,
// call it directly (kernel32 is always loaded, no QLibrary needed).
QString userTag() {
  DWORD sessionId = 0;
  if(!ProcessIdToSessionId(GetCurrentProcessId(), &sessionId))
    qCWarning(lmcApp(), "SingleInstance: ProcessIdToSessionId failed: %lu", GetLastError());
  return QString::number(sessionId, 16);
}
#else
QString userTag() {
  return QString::number(::getuid(), 16);
}
#endif

} // namespace

SingleInstance::SingleInstance(const QString& appId, QObject* parent)
    : QObject(parent),
      inFlatpak_(inFlatpakSandbox()),
      socketName_(socketNameFor(appId)),
      serverPath_(serverPathFor(socketName_)),
      lockFile_(lockFileFor(socketName_)) {
  server_ = new QLocalServer(this);
  if(inFlatpak_) {
    const QString ipcDir = flatpakIpcDir();
    if(!QDir().mkpath(ipcDir))
      qCWarning(lmcApp(), "SingleInstance: cannot create IPC dir '%s'", qPrintable(ipcDir));
  }
  connect(server_, &QLocalServer::newConnection, this, &SingleInstance::receiveConnection);
}

SingleInstance::~SingleInstance() {
  if(server_)
    server_->close();
  if(lockFile_.isLocked())
    lockFile_.unlock();
}

QString SingleInstance::socketNameFor(const QString& appId) {
  // The uid suffix keeps per-user isolation. 32 bits of SHA-256: stable
  // across Qt versions.
  const QByteArray digest =
    QCryptographicHash::hash(appId.toUtf8(), QCryptographicHash::Sha256);
  const QString idHex =
    QString::fromLatin1(digest.left(4).toHex()); // 8 hex chars, 32 bits
  return QStringLiteral("lmcsi-") + idHex + QLatin1Char('-') + userTag();
}

QString SingleInstance::lockFileFor(const QString& socketName) const {
  if(inFlatpak_)
    return QDir(flatpakIpcDir()).absoluteFilePath(socketName + QStringLiteral("-lockfile"));
  return QDir(QDir::tempPath()).absoluteFilePath(socketName + QStringLiteral("-lockfile"));
}

QString SingleInstance::serverPathFor(const QString& socketName) const {
  if(inFlatpak_)
    return QDir(flatpakIpcDir()).absoluteFilePath(socketName);
  return socketName;
}

bool SingleInstance::isServer() const {
  return lockFile_.isLocked() && listening_;
}

QString SingleInstance::endpoint() const {
  return serverPath_;
}

bool SingleInstance::isLockedByOther() {
  // Self-hold means no *other* holder (the lock is exclusive).
  if(lockFile_.isLocked())
    return false;
  // Separate probe: never touches our own guard, never elects via listen().
  QLockFile probe(lockFile_.fileName());
  if(probe.tryLock()) {
    probe.unlock();
    return false;
  }
  return true;
}

bool SingleInstance::tryDeliver(const QString& message, int timeout) {
  Q_ASSERT_X(!isServer(), "SingleInstance::tryDeliver", "call only from the HeldByOther branch");
  QLocalSocket socket;
  bool connOk = false;
  for(int i = 0; i < 2; i++) {
    // Try twice, in case the other instance is just starting up.
    socket.connectToServer(serverPath_);
    connOk = socket.waitForConnected(timeout / 2);
    if(connOk || i)
      break;
    QThread::msleep(250);
  }
  if(!connOk)
    return false;
  const char opcode = opcodeFor(message);
  if(socket.write(&opcode, 1) != 1)
    return false;
  bool res = socket.waitForBytesWritten(timeout);
  res = res && socket.waitForReadyRead(timeout);
  char ack = 0;
  res = res && (socket.read(&ack, 1) == 1 && ack == kAckOpcode);
  return res;
}

bool SingleInstance::ensureServer(int retries, int sleepMs) {
  // Heals the transient-holder race within retries*sleepMs. Only the first
  // and the last attempt warn; the caller owns the final loud warning.
  for(int i = 0; i <= retries; i++) {
    if(tryBecomeServer(i == 0 || i == retries) == Election::Elected)
      return true;
    if(i < retries)
      QThread::msleep(sleepMs);
  }
  return false;
}

SingleInstance::StartupOutcome SingleInstance::resolveStartup(const QString& remoteCommand,
                                                              int retries, int sleepMs,
                                                              int timeout) {
  // Single probe: cold start elects here with no socket waits. Delivery
  // gets one attempt with the full timeout; the loop below is
  // election-only.
  const Election role = tryBecomeServer(true);
  if(role == Election::HeldByOther) {
    if(tryDeliver(remoteCommand, timeout))
      return StartupOutcome::SecondDelivered;
  } else if(role == Election::Elected) {
    return StartupOutcome::FirstServer;
  }
  for(int i = 0; i < retries; i++) {
    if(tryBecomeServer(i == retries - 1) == Election::Elected)
      return StartupOutcome::FirstServer;
    if(i + 1 < retries)
      QThread::msleep(sleepMs);
  }
  return StartupOutcome::Pathology;
}

SingleInstance::Election SingleInstance::tryBecomeServer(bool warn) {
  if(lockFile_.isLocked() && listening_)
    return Election::Elected;
  if(!lockFile_.tryLock())
    return Election::HeldByOther;
  if(listenWithRescue()) {
    listening_ = true;
    return Election::Elected;
  }
  // Pathology: lock ours, socket unlistenable. Release it, warn, degrade.
  if(warn)
    qCWarning(lmcApp(), "SingleInstance: lock acquired but cannot listen on '%s': %s",
              qPrintable(serverPath_),
              qPrintable(server_ ? server_->errorString() : QString()));
  lockFile_.unlock();
  return Election::ListenFailed;
}

bool SingleInstance::listenWithRescue() {
  if(server_->isListening())
    return true;
  if(server_->listen(serverPath_))
    return true;
  if(server_->serverError() != QAbstractSocket::AddressInUseError)
    return false;
  // Stale socket file; lock-gated, can't hijack a live server.
  QLocalServer::removeServer(serverPath_);
  return server_->listen(serverPath_);
}

void SingleInstance::receiveConnection() {
  while(server_->hasPendingConnections()) {
    QLocalSocket* socket = server_->nextPendingConnection();
    if(!socket)
      continue;
    // Failures drop the peer without ack.
    QDeadlineTimer deadline(kOpcodeTimeoutMs);
    const auto drop = [socket](const QString& why) {
      qCWarning(lmcApp(), "SingleInstance: dropping peer message: %s", qPrintable(why));
      delete socket;
    };
    while(socket->bytesAvailable() < 1) {
      const int left = static_cast<int>(deadline.remainingTime());
      if(left <= 0 || !socket->waitForReadyRead(left)) {
        drop(QStringLiteral("opcode timeout"));
        socket = nullptr;
        break;
      }
    }
    if(!socket)
      continue;
    char opcode = 0;
    if(socket->read(&opcode, 1) != 1) {
      drop(QStringLiteral("opcode timeout"));
      continue;
    }
    if(opcode != kRaiseOpcode && opcode != kTermOpcode) {
      drop(QStringLiteral("unknown opcode"));
      continue;
    }
    const QString message(messageForOpcode(opcode));
    socket->write(&kAckOpcode, 1);
    socket->waitForBytesWritten(1000);
    delete socket;
    qCDebug(lmcApp(), "SingleInstance: dispatching '%s' command", qPrintable(message));
    emit messageReceived(message);
  }
}
