// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Unit tests for Application::event: a macOS Dock/Finder reopen spawns no
// second process, it arrives in-process as QEvent::ApplicationActivate and
// must raise through the same messageReceived("raise") path as IPC.
#include <QtTest>

#include "app/application.h"

class TstApplication : public QObject {
  Q_OBJECT
private slots:
  void reopenRaisesOrIgnores(void);
};

void TstApplication::reopenRaisesOrIgnores(void) {
  auto* app = qobject_cast<Application*>(QCoreApplication::instance());
  QVERIFY(app != nullptr);
  QSignalSpy spy(app, &Application::messageReceived);

  QEvent reopen(QEvent::ApplicationActivate);
  QCoreApplication::sendEvent(app, &reopen);

#ifdef Q_OS_MAC
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("raise"));
#else
  // No behavior change off macOS: focus regain must stay silent.
  QCOMPARE(spy.count(), 0);
#endif
}

int main(int argc, char** argv) {
  // QTEST_MAIN would instantiate its own QApplication; the event() override
  // under test lives on Application, so own the instance here and drive
  // QTest against it. Headless: QT_QPA_PLATFORM=offscreen (ctest property).
  Application app(QStringLiteral("tst-application-reopen"), argc, argv);
  TstApplication tc;
  return QTest::qExec(&tc, argc, argv);
}
#include "tst_application.moc"
