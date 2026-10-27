// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Unit tests for remoteCommandFor: second-instance argv -> IPC command.
#include <QtTest>

#include "app/apprunner.h"

class TstApprunner : public QObject {
  Q_OBJECT
private slots:
  void plainLaunchRaises(void);
  void termCloses(void);
  void termwaitCloses(void);
  void termCaseInsensitive(void);
  void startupOnlySwitchesRaise(void);
  void termWinsOverOthers(void);
  void waitOnlyForTermwait(void);
};

void TstApprunner::plainLaunchRaises(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc"}), QStringLiteral("raise"));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/quit"}), QStringLiteral("raise"));
}

void TstApprunner::termCloses(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/term"}), QStringLiteral("term"));
  QVERIFY(!wantsTermWait(QStringList{"/usr/bin/lmc", "/term"}));
}

void TstApprunner::termwaitCloses(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/termwait"}), QStringLiteral("term"));
  QVERIFY(wantsTermWait(QStringList{"/usr/bin/lmc", "/termwait"}));
  QVERIFY(wantsTermWait(QStringList{"/usr/bin/lmc", "/TERMWAIT"}));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/term", "/termwait"}),
           QStringLiteral("term"));
  QVERIFY(wantsTermWait(QStringList{"/usr/bin/lmc", "/term", "/termwait"}));
}

void TstApprunner::termCaseInsensitive(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/TERM"}), QStringLiteral("term"));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/Term"}), QStringLiteral("term"));
}

void TstApprunner::startupOnlySwitchesRaise(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/loopback"}), QStringLiteral("raise"));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/loopback", "/noconfig"}),
           QStringLiteral("raise"));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/nohistory"}), QStringLiteral("raise"));
}

void TstApprunner::termWinsOverOthers(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/loopback", "/term"}),
           QStringLiteral("term"));
  QVERIFY(!wantsTermWait(QStringList{"/usr/bin/lmc", "/loopback", "/term"}));
}

void TstApprunner::waitOnlyForTermwait(void) {
  QVERIFY(!wantsTermWait(QStringList{"/usr/bin/lmc"}));
  QVERIFY(!wantsTermWait(QStringList{"/usr/bin/lmc", "/quit"}));
  QVERIFY(!wantsTermWait(QStringList{"/usr/bin/lmc", "/loopback", "/noconfig"}));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/termwaitfoo"}), QStringLiteral("raise"));
  QVERIFY(!wantsTermWait(QStringList{"/usr/bin/lmc", "/termwaitfoo"}));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/terminal"}), QStringLiteral("raise"));
}

QTEST_MAIN(TstApprunner)
#include "tst_apprunner.moc"
