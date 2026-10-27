// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Unit tests for SingleInstance. The binary re-executes itself as a helper:
//   --probe-elect <appId> <holdMs>  elect via tryBecomeServer(), linger, exit
//   --si-holder <appId> <secs>      live server, prints READY then GOT:<msg>
#include <QtTest>

#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QLocalSocket>
#include <QProcess>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <atomic>

#include "app/singleinstance.h"

namespace {

QString freshAppId(const char* tag) {
  return QStringLiteral("tst-singleinstance-%1-%2")
    .arg(QLatin1String(tag), QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QString selfBinary() {
  return QCoreApplication::applicationFilePath();
}

bool waitForLine(QProcess& child, const QByteArray& prefix, int timeoutMs, QByteArray* got = nullptr) {
  const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + timeoutMs;
  QByteArray buf;
  while(QDateTime::currentMSecsSinceEpoch() < deadline) {
    if(!child.waitForReadyRead(250))
      continue;
    buf += child.readAllStandardOutput();
    int nl = -1;
    while((nl = buf.indexOf('\n')) >= 0) {
      const QByteArray line = buf.left(nl).trimmed();
      buf = buf.mid(nl + 1);
      if(line.startsWith(prefix)) {
        if(got)
          *got = line;
        return true;
      }
    }
  }
  return false;
}

int runProbeElect(const QStringList& args) {
  QTextStream out(stdout);
  SingleInstance probe(args.value(1));
  const bool heldByOther = probe.tryBecomeServer() == SingleInstance::Election::HeldByOther;
  out << (heldByOther ? "CLIENT\n" : "READY\n");
  out.flush();
  if(heldByOther)
    return 1; // lost the election: not the scenario under test
  QThread::msleep(static_cast<unsigned>(args.value(2).toUInt()));
  return 0;
}

int runSiHolder(const QStringList& args, QCoreApplication& app) {
  QTextStream out(stdout);
  SingleInstance holder(args.value(1));
  if(!holder.ensureServer(100, 100)) {
    out << "FAIL\n";
    out.flush();
    return 1;
  }
  QObject::connect(&holder, &SingleInstance::messageReceived, [&](const QString& msg) {
    out << "GOT:" << msg << "\n";
    out.flush();
  });
  out << "READY\n";
  out.flush();
  QTimer::singleShot(args.value(2).toInt() * 1000, &app, &QCoreApplication::quit);
  return app.exec();
}

} // namespace

// Blocking client living fully inside a worker thread.
class SenderThread : public QThread {
  Q_OBJECT
public:
  SenderThread(const QString& appId, const QString& message, QObject* parent = nullptr)
      : QThread(parent), appId_(appId), message_(message) {}
  std::atomic_bool ok{false};

protected:
  void run() override {
    SingleInstance client(appId_);
    ok = client.tryDeliver(message_);
  }

private:
  QString appId_;
  QString message_;
};

class TstSingleInstance : public QObject {
  Q_OBJECT
private slots:
  void firstBecomesServer(void);
  void secondSeesClient(void);
  void deliveryAck(void);
  void noListenerSendsFalse(void);
  void transientHeal(void);
  void resolveColdStartFirst(void);
  void resolveSecondDelivers(void);
  void resolveWedgedPathology(void);
  void persistentLockFailsBudget(void);
  void destructorReleases(void);
  void killTakeover(void);
  void unknownOpcodeDropped(void);
  void emptyDisconnectDropped(void);
  void peekSeesLiveHolder(void);
  void peekFreeWhenNobody(void);
  void peekFreeAfterRelease(void);
};

void TstSingleInstance::firstBecomesServer(void) {
  SingleInstance a(freshAppId("first"));
  QVERIFY2(a.tryBecomeServer() == SingleInstance::Election::Elected,
           "first instance must not see another");
  QVERIFY2(a.isServer(), "first instance must hold the lock and listen");
}

void TstSingleInstance::secondSeesClient(void) {
  const QString id = freshAppId("second");
  SingleInstance a(id);
  QVERIFY2(a.tryBecomeServer() == SingleInstance::Election::Elected, "first must elect");
  SingleInstance b(id);
  QVERIFY2(b.tryBecomeServer() == SingleInstance::Election::HeldByOther,
           "second must see the first");
  QVERIFY2(!b.isServer(), "second must not be the server");
  QVERIFY2(a.isServer(), "first stays the server");
}

void TstSingleInstance::deliveryAck(void) {
  const QString id = freshAppId("delivery");
  QProcess child;
  child.start(selfBinary(), {"--si-holder", id, "30"});
  QVERIFY2(child.waitForStarted(5000), "holder child failed to start");
  QVERIFY2(waitForLine(child, "READY", 15000), "holder did not elect");
  SingleInstance client(id);
  QVERIFY2(client.tryBecomeServer() == SingleInstance::Election::HeldByOther,
           "client must see the holder");
  QVERIFY2(client.tryDeliver(QStringLiteral("raise")), "server must ack raise");
  QByteArray got;
  QVERIFY2(waitForLine(child, "GOT:", 15000, &got), "server did not receive raise");
  QCOMPARE(got, QByteArray("GOT:raise"));
  QVERIFY2(client.tryDeliver(QStringLiteral("term")), "server must ack term");
  QVERIFY2(waitForLine(child, "GOT:", 15000, &got), "server did not receive term");
  QCOMPARE(got, QByteArray("GOT:term"));
  child.kill();
  QVERIFY2(child.waitForFinished(5000), "holder did not die");
}

void TstSingleInstance::noListenerSendsFalse(void) {
  // Nobody holds this id: pure delivery reports false without electing us.
  SingleInstance c(freshAppId("nolistener"));
  QVERIFY2(!c.tryDeliver(QStringLiteral("nobody")), "no listener must yield false");
  QVERIFY2(!c.isServer(), "failed delivery must not elect");
}

void TstSingleInstance::transientHeal(void) {
  // Transient winner exits, first heals.
  const QString id = freshAppId("heal");
  QProcess probe;
  probe.start(selfBinary(), {"--probe-elect", id, "1500"});
  QVERIFY2(probe.waitForStarted(5000), "probe failed to start");
  QVERIFY2(waitForLine(probe, "READY", 15000), "probe did not elect");
  SingleInstance first(id);
  QVERIFY2(first.tryBecomeServer() == SingleInstance::Election::HeldByOther,
           "lock is held by the transient probe");
  // Probe listens but never acks: delivery must be false.
  QVERIFY2(!first.tryDeliver(QStringLiteral("racy"), 1000), "transient must not ack");
  QVERIFY2(probe.waitForFinished(10000), "probe did not exit");
  QVERIFY2(first.ensureServer(100, 100), "first must heal after transient exit");
  QVERIFY2(first.isServer(), "first must listen after heal");
}

void TstSingleInstance::resolveColdStartFirst(void) {
  SingleInstance a(freshAppId("resolvefirst"));
  QVERIFY2(a.resolveStartup(QStringLiteral("raise"), 5, 50, 1000) ==
             SingleInstance::StartupOutcome::FirstServer,
           "cold start must elect");
  QVERIFY2(a.isServer(), "resolved first must serve");
}

void TstSingleInstance::resolveSecondDelivers(void) {
  const QString id = freshAppId("resolvedeliver");
  QProcess child;
  child.start(selfBinary(), {"--si-holder", id, "30"});
  QVERIFY2(child.waitForStarted(5000), "holder child failed to start");
  QVERIFY2(waitForLine(child, "READY", 15000), "holder did not elect");
  SingleInstance me(id);
  QVERIFY2(me.resolveStartup(QStringLiteral("raise"), 10, 100, 5000) ==
             SingleInstance::StartupOutcome::SecondDelivered,
           "live holder must be delivered");
  QVERIFY2(!me.isServer(), "resolved second must not serve");
  QByteArray got;
  QVERIFY2(waitForLine(child, "GOT:", 15000, &got), "server did not receive raise");
  QCOMPARE(got, QByteArray("GOT:raise"));
  child.kill();
  QVERIFY2(child.waitForFinished(5000), "holder did not die");
}

void TstSingleInstance::resolveWedgedPathology(void) {
  // Holder listens but never acks and never exits: budget must degrade.
  const QString id = freshAppId("resolvewedged");
  QProcess holder;
  holder.start(selfBinary(), {"--probe-elect", id, "60000"});
  QVERIFY2(holder.waitForStarted(5000), "holder failed to start");
  QVERIFY2(waitForLine(holder, "READY", 15000), "holder did not elect");
  SingleInstance me(id);
  QElapsedTimer t;
  t.start();
  QVERIFY2(me.resolveStartup(QStringLiteral("raise"), 3, 100, 1000) ==
             SingleInstance::StartupOutcome::Pathology,
           "wedged holder must degrade");
  QVERIFY2(t.elapsed() < 15000, "resolveStartup must stay bounded");
  holder.kill();
  QVERIFY2(holder.waitForFinished(5000), "holder did not die");
}

void TstSingleInstance::persistentLockFailsBudget(void) {
  const QString id = freshAppId("persistent");
  QProcess holder;
  holder.start(selfBinary(), {"--probe-elect", id, "60000"});
  QVERIFY2(holder.waitForStarted(5000), "holder failed to start");
  QVERIFY2(waitForLine(holder, "READY", 15000), "holder did not elect");
  SingleInstance first(id);
  QVERIFY2(first.tryBecomeServer() == SingleInstance::Election::HeldByOther,
           "holder must be visible");
  QElapsedTimer t;
  t.start();
  QVERIFY2(!first.ensureServer(5, 100), "persistent lock must fail the budget");
  QVERIFY2(t.elapsed() < 10000, "ensureServer must stay bounded");
  holder.kill();
  QVERIFY2(holder.waitForFinished(5000), "holder did not die");
}

void TstSingleInstance::destructorReleases(void) {
  const QString id = freshAppId("dtor");
  {
    SingleInstance a(id);
    QVERIFY2(a.tryBecomeServer() == SingleInstance::Election::Elected,
             "setup: first must elect");
  }
  SingleInstance b(id);
  QVERIFY2(b.tryBecomeServer() == SingleInstance::Election::Elected,
           "lock must be free after destructor");
  QVERIFY2(b.isServer(), "second must elect after release");
}

void TstSingleInstance::killTakeover(void) {
  const QString id = freshAppId("takeover");
  QProcess child;
  child.start(selfBinary(), {"--si-holder", id, "60"});
  QVERIFY2(child.waitForStarted(5000), "holder child failed to start");
  QVERIFY2(waitForLine(child, "READY", 15000), "holder did not elect");
  SingleInstance first(id);
  QVERIFY2(first.tryBecomeServer() == SingleInstance::Election::HeldByOther,
           "holder must be visible");
  child.kill();
  QVERIFY2(child.waitForFinished(5000), "holder did not die");
  QVERIFY2(first.ensureServer(100, 100), "crashed holder must be taken over");
  QVERIFY2(first.isServer(), "first must listen after takeover");
  // The client object lives fully in the worker thread.
  QSignalSpy spy(&first, &SingleInstance::messageReceived);
  SenderThread sender(id, QStringLiteral("raise"));
  sender.start();
  QEventLoop loop;
  QTimer::singleShot(15000, &loop, &QEventLoop::quit);
  connect(&sender, &QThread::finished, &loop, &QEventLoop::quit);
  loop.exec();
  QVERIFY2(sender.wait(15000), "sender thread hung");
  QVERIFY2(sender.ok.load(), "takeover server must ack");
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.first().first().toString(), QStringLiteral("raise"));
}

void TstSingleInstance::unknownOpcodeDropped(void) {
  const QString id = freshAppId("unknownop");
  QProcess child;
  child.start(selfBinary(), {"--si-holder", id, "30"});
  QVERIFY2(child.waitForStarted(5000), "holder child failed to start");
  QVERIFY2(waitForLine(child, "READY", 15000), "holder did not elect");
  const QString name = SingleInstance(id).socketName(); // no election here
  {
    QLocalSocket raw;
    raw.connectToServer(name);
    QVERIFY2(raw.waitForConnected(5000), "raw connect failed");
    QVERIFY2(raw.write("?", 1) == 1, "unknown opcode not queued");
    QVERIFY2(raw.waitForBytesWritten(5000), "unknown opcode not written");
    QVERIFY2(!raw.waitForReadyRead(3000), "unknown opcode must not be acked");
  }
  SingleInstance client(id);
  QVERIFY2(client.tryDeliver(QStringLiteral("raise")), "server must serve after unknown opcode");
  QByteArray got;
  QVERIFY2(waitForLine(child, "GOT:", 15000, &got), "server did not receive post-drop");
  QCOMPARE(got, QByteArray("GOT:raise"));
  child.kill();
  QVERIFY2(child.waitForFinished(5000), "holder did not die");
}

void TstSingleInstance::emptyDisconnectDropped(void) {
  const QString id = freshAppId("emptydrop");
  QProcess child;
  child.start(selfBinary(), {"--si-holder", id, "30"});
  QVERIFY2(child.waitForStarted(5000), "holder child failed to start");
  QVERIFY2(waitForLine(child, "READY", 15000), "holder did not elect");
  const QString name = SingleInstance(id).socketName(); // no election here
  {
    QLocalSocket raw;
    raw.connectToServer(name);
    QVERIFY2(raw.waitForConnected(5000), "raw connect failed");
    raw.disconnectFromServer();
    QVERIFY2(!raw.waitForReadyRead(3000), "empty disconnect must not be acked");
  }
  SingleInstance client(id);
  QVERIFY2(client.tryDeliver(QStringLiteral("term")), "server must serve after empty disconnect");
  QByteArray got;
  QVERIFY2(waitForLine(child, "GOT:", 15000, &got), "server did not receive post-drop");
  QCOMPARE(got, QByteArray("GOT:term"));
  child.kill();
  QVERIFY2(child.waitForFinished(5000), "holder did not die");
}

void TstSingleInstance::peekSeesLiveHolder(void) {
  const QString id = freshAppId("peekholder");
  SingleInstance holder(id);
  QVERIFY2(holder.ensureServer(100, 100), "holder must elect");
  SingleInstance probe(id);
  QVERIFY2(probe.isLockedByOther(), "peek must see the live holder");
  QVERIFY2(!probe.isServer(), "peek must not elect");
  QVERIFY2(holder.isServer(), "holder must stay the server after probe peek");
  QVERIFY2(!holder.isLockedByOther(), "self-hold is not 'other'");
}

void TstSingleInstance::peekFreeWhenNobody(void) {
  const QString id = freshAppId("peekfree");
  SingleInstance probe(id);
  QVERIFY2(!probe.isLockedByOther(), "peek must report free");
  QVERIFY2(!probe.isServer(), "peek must not elect");
}

void TstSingleInstance::peekFreeAfterRelease(void) {
  const QString id = freshAppId("peekrelease");
  {
    SingleInstance holder(id);
    QVERIFY2(holder.ensureServer(100, 100), "holder must elect");
    SingleInstance probe(id);
    QVERIFY2(probe.isLockedByOther(), "holder must be visible");
    QVERIFY2(holder.isServer(), "holder must stay the server after probe peek");
  }
  SingleInstance after(id);
  QVERIFY2(!after.isLockedByOther(), "lock must be free after destructor");
  QVERIFY2(!after.isServer(), "peek must not elect");
}

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  const QStringList args = app.arguments();
  if(args.contains(QStringLiteral("--probe-elect")))
    return runProbeElect(args.mid(args.indexOf(QStringLiteral("--probe-elect"))));
  if(args.contains(QStringLiteral("--si-holder")))
    return runSiHolder(args.mid(args.indexOf(QStringLiteral("--si-holder"))), app);
  TstSingleInstance t;
  return QTest::qExec(&t, argc, argv);
}

#include "tst_singleinstance.moc"
