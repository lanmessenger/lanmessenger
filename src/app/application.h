// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QApplication>
#include <QHash>
#include <QStringList>
#include "singleinstance.h"
class QDir;
class QEvent;
class QTranslator;

typedef QHash<QString, QTranslator*> Translators;

class Application : public QApplication {
  Q_OBJECT

public:
  explicit Application(const QString& id, int& argc, char** argv);
  ~Application() override;

  bool isRunning();
  SingleInstance::StartupOutcome resolveStartup(const QString& remoteCommand, int retries,
                                                int sleepMs, int timeout = 5000);

  static void loadTranslations(const QString& dir);
  static void loadTranslations(const QDir& dir);
  static const QStringList availableLanguages();

public slots:
  static void setLanguage(const QString& locale);

Q_SIGNALS:
  void messageReceived(const QString& message);

protected:
  bool event(QEvent* e) override;

private:
  SingleInstance* single_ = nullptr;

  static QTranslator* current;
  static QTranslator* sysCurrent;
  static Translators translators;
  static Translators sysTranslators;
};
