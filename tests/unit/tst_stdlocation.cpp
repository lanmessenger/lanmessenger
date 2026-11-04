// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "core/stdlocation.h"

class TstStdLocation : public QObject {
  Q_OBJECT
private slots:
  void normalizeKeepsAbsolute(void);
  void normalizeCollapsesLegacy(void);
  void normalizeLeavesOtherRelative(void);
  void resolvePrefersFirstExisting(void);
  void resolveBothExistPrefersFirst(void);
  void resolveFallsBackToFirst(void);
  void dataCandidatesOrder(void);
};

void TstStdLocation::normalizeKeepsAbsolute(void) {
  const QString abs = QDir::toNativeSeparators(QStringLiteral("/home/user/custom.wav"));
  QCOMPARE(StdLocation::normalizeSoundFile(abs), abs);
  QVERIFY(StdLocation::normalizeSoundFile(QString()).isEmpty());
}

void TstStdLocation::normalizeCollapsesLegacy(void) {
  QCOMPARE(StdLocation::normalizeSoundFile(QStringLiteral("./sounds/newmessage.wav")),
           QStringLiteral("newmessage.wav"));
  QCOMPARE(StdLocation::normalizeSoundFile(QStringLiteral("sounds/useronline.wav")),
           QStringLiteral("useronline.wav"));
  QCOMPARE(StdLocation::normalizeSoundFile(QStringLiteral("newmessage.wav")),
           QStringLiteral("newmessage.wav"));
}

void TstStdLocation::normalizeLeavesOtherRelative(void) {
  QCOMPARE(StdLocation::normalizeSoundFile(QStringLiteral("my-sounds/foo.wav")),
           QStringLiteral("my-sounds/foo.wav"));
  QCOMPARE(StdLocation::normalizeSoundFile(QStringLiteral("../foo.wav")),
           QStringLiteral("../foo.wav"));
}

void TstStdLocation::resolvePrefersFirstExisting(void) {
  QTemporaryDir first;
  QVERIFY(first.isValid());
  QTemporaryDir second;
  QVERIFY(second.isValid());
  QFile f(second.path() + QStringLiteral("/newmessage.wav"));
  QVERIFY(f.open(QIODevice::WriteOnly));
  f.close();
  QCOMPARE(StdLocation::resolveSoundFileFrom(QStringLiteral("newmessage.wav"),
                                             QStringList{first.path(), second.path()}),
           QDir::toNativeSeparators(second.path() + QStringLiteral("/newmessage.wav")));
  QCOMPARE(StdLocation::resolveSoundFileFrom(QStringLiteral("./sounds/newmessage.wav"),
                                             QStringList{first.path(), second.path()}),
           QDir::toNativeSeparators(second.path() + QStringLiteral("/newmessage.wav")));
  const QString abs = QDir::toNativeSeparators(first.path() + QStringLiteral("/custom.wav"));
  QCOMPARE(StdLocation::resolveSoundFileFrom(abs, QStringList{second.path()}), abs);
}

void TstStdLocation::resolveBothExistPrefersFirst(void) {
  QTemporaryDir first;
  QVERIFY(first.isValid());
  QTemporaryDir second;
  QVERIFY(second.isValid());
  for(const QString& dir : {first.path(), second.path()}) {
    QFile f(dir + QStringLiteral("/newmessage.wav"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.close();
  }
  QCOMPARE(StdLocation::resolveSoundFileFrom(QStringLiteral("newmessage.wav"),
                                             QStringList{first.path(), second.path()}),
           QDir::toNativeSeparators(first.path() + QStringLiteral("/newmessage.wav")));
}

void TstStdLocation::resolveFallsBackToFirst(void) {
  QTemporaryDir first;
  QVERIFY(first.isValid());
  QTemporaryDir second;
  QVERIFY(second.isValid());
  QCOMPARE(StdLocation::resolveSoundFileFrom(QStringLiteral("missing.wav"),
                                             QStringList{first.path(), second.path()}),
           QDir::toNativeSeparators(first.path() + QStringLiteral("/missing.wav")));
}

void TstStdLocation::dataCandidatesOrder(void) {
  const QStringList bases = StdLocation::dataCandidateDirs();
  const QString appDir = QDir::cleanPath(QCoreApplication::applicationDirPath());
  QVERIFY(!appDir.isEmpty());
  QVERIFY(!bases.isEmpty());
  QCOMPARE(bases.at(0), appDir);
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
  QVERIFY(bases.size() >= 2);
  QCOMPARE(bases.at(1), QDir::cleanPath(appDir + QStringLiteral("/../share/lmc")));
#endif
}

QTEST_MAIN(TstStdLocation)
#include "tst_stdlocation.moc"
