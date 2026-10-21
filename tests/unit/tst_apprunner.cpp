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
  void termCaseInsensitive(void);
  void startupOnlySwitchesRaise(void);
  void termWinsOverOthers(void);
};

void TstApprunner::plainLaunchRaises(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc"}), QStringLiteral("raise"));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/quit"}), QStringLiteral("raise"));
}

void TstApprunner::termCloses(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/term"}), QStringLiteral("term"));
}

void TstApprunner::termCaseInsensitive(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/TERM"}), QStringLiteral("term"));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/Term"}), QStringLiteral("term"));
}

void TstApprunner::startupOnlySwitchesRaise(void) {
  // Init-only switches must not leak into IPC: the running instance just raises.
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/loopback"}), QStringLiteral("raise"));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/loopback", "/noconfig"}),
           QStringLiteral("raise"));
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/nohistory"}), QStringLiteral("raise"));
}

void TstApprunner::termWinsOverOthers(void) {
  QCOMPARE(remoteCommandFor(QStringList{"/usr/bin/lmc", "/loopback", "/term"}),
           QStringLiteral("term"));
}

QTEST_MAIN(TstApprunner)
#include "tst_apprunner.moc"
