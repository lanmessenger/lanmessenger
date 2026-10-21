// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#ifndef LMC_SINGLEINSTANCE_H
#define LMC_SINGLEINSTANCE_H

#include <QObject>
#include <QLockFile>
#include <QString>

class QLocalServer;

// Single-instance guard on QLockFile + QLocalServer. The tryLock() loser is
// the client, the winner must listen(). Wire format is a 1-byte opcode
// ('R' = raise, 'T' = term) with a 1-byte ack ('A'); the "lmcsi-*"
// namespace is disjoint from "qtsingleapp-*" by design: no
// cross-version single-instance guarantee.
class SingleInstance : public QObject {
  Q_OBJECT
public:
  // ListenFailed releases the lock: nothing actually runs.
  enum class Election {
    Elected,
    HeldByOther,
    ListenFailed
  };
  // Pathology means the election budget ran out: warn loudly and continue
  // degraded, without the guard.
  enum class StartupOutcome {
    FirstServer,
    SecondDelivered,
    Pathology
  };

  explicit SingleInstance(const QString& appId, QObject* parent = nullptr);
  ~SingleInstance() override;

  // QLockFile has no peek: probing via tryLock() elects us on success.
  Election tryBecomeServer(bool warn = true);
  // True if this instance currently holds the lock AND listens.
  bool isServer() const;
  // /inst contract: 1 iff the lock is held by someone else.
  bool isRunning();
  // Local socket name (test hook).
  QString socketName() const;
  // No election inside: call only from the HeldByOther branch.
  // Worst case ~15 s on a wedged lock-holder.
  bool tryDeliver(const QString& message, int timeout = 5000);
  // Budget retries*sleepMs; false = pathology or a wedged holder.
  bool ensureServer(int retries, int sleepMs);
  // Probe once, then branch; heals the transient-holder race within
  // retries*sleepMs.
  StartupOutcome resolveStartup(const QString& remoteCommand, int retries, int sleepMs,
                                int timeout = 5000);
Q_SIGNALS:
  void messageReceived(const QString& message);

private:
  bool listenWithRescue();
  void receiveConnection();
  static QString socketNameFor(const QString& appId);
  static QString lockFileFor(const QString& socketName);

  QLocalServer* server_ = nullptr;
  QString socketName_;
  // After socketName_: the lock path derives from it.
  QLockFile lockFile_;
  bool listening_ = false;
};

#endif // LMC_SINGLEINSTANCE_H
