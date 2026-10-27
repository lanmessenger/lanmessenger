// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
// Copyright (c) 2026 LAN Messenger.
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QTranslator>
#include "application.h"
#include "singleinstance.h"

#define IDS_LANGUAGE_VAL "en_US"

QTranslator* Application::current = nullptr;
QTranslator* Application::sysCurrent = nullptr;
Translators Application::translators;
Translators Application::sysTranslators;

Application::Application(const QString& id, int& argc, char** argv)
    : QApplication(argc, argv), single_(new SingleInstance(id, this)) {
  connect(single_, &SingleInstance::messageReceived, this, &Application::messageReceived);
}

Application::~Application() {
}

SingleInstance::Election Application::tryBecomeServer(bool warn) {
  return single_->tryBecomeServer(warn);
}

bool Application::isLockedByOther() {
  return single_->isLockedByOther();
}

bool Application::event(QEvent* e) {
#ifdef Q_OS_MAC
  // A macOS Dock/Finder reopen spawns no second process: LaunchServices
  // delivers it in-process as ApplicationActivate. Route it through the same
  // "raise" path as a second-instance IPC delivery.
  if(e->type() == QEvent::ApplicationActivate)
    emit messageReceived(QStringLiteral("raise"));
#endif
  return QApplication::event(e);
}

SingleInstance::StartupOutcome Application::resolveStartup(const QString& remoteCommand,
                                                           int retries, int sleepMs,
                                                           int timeout) {
  return single_->resolveStartup(remoteCommand, retries, sleepMs, timeout);
}

void Application::loadTranslations(const QString& dir) {
  loadTranslations(QDir(dir));
}

void Application::loadTranslations(const QDir& dir) {
  // <language>_<country>
  QString filter = "*_*.qm";
  QDir::Filters filters = QDir::Files | QDir::Readable;
  QDir::SortFlags sort = QDir::Name;
  QFileInfoList entries = dir.entryInfoList(QStringList() << filter, filters, sort);
  // Locales found in this directory. System translations below are loaded
  // per directory (not from the global hash), so user-supplied
  // <userLangDir>/system/*.qm keep working.
  QStringList locales;
  for(QFileInfo file : entries) {
    // pick country and language out of the file name
    QStringList parts = file.baseName().split("_");
    if(parts.count() < 2)
      continue;
    QString language = parts.at(parts.count() - 2).toLower();
    QString country = parts.at(parts.count() - 1).toUpper();

    // construct and load translator
    QTranslator* translator = new QTranslator(instance());
    if(translator->load(file.absoluteFilePath())) {
      QString locale = language + "_" + country;
      delete translators.take(locale);
      translators.insert(locale, translator);
      if(!locales.contains(locale))
        locales.append(locale);
    } else
      delete translator;
  }
  // en_US is the source language: keep the nullptr marker (no app translator),
  // dropping the just-loaded identity translator if any.
  delete translators.take(IDS_LANGUAGE_VAL);
  translators.insert(IDS_LANGUAGE_VAL, nullptr);

  // Qt's own translations for the standard dialogs (qtbase_*.qm, bundled at
  // build time): country specific file first (qtbase_pt_BR.qm), language
  // only as fallback (qtbase_pt.qm).
  QDir sysDir(dir.absolutePath() + "/system");
  if(!sysDir.exists())
    return;

  for(const QString& locale : locales) {
    // en_US is the source language (nullptr translator); there is no
    // qtbase_en*.qm for it, mirroring the CMake skip.
    if(locale == IDS_LANGUAGE_VAL)
      continue;
    QStringList parts = locale.split("_");
    QString language = QString("qtbase_%1").arg(parts.value(0).toLower());
    QString country = QString("qtbase_%1_%2")
                        .arg(parts.value(0).toLower(), parts.value(1).toUpper());

    QTranslator* translator = new QTranslator(instance());
    if(translator->load(country, sysDir.absolutePath()) ||
       translator->load(language, sysDir.absolutePath())) {
      delete sysTranslators.take(locale);
      sysTranslators.insert(locale, translator);
    } else
      delete translator;
  }
}

const QStringList Application::availableLanguages() {
  // the content won't get copied thanks to implicit sharing and constness
  return QStringList(translators.keys());
}

void Application::setLanguage(const QString& locale) {
  // remove previous translator
  if(current)
    removeTranslator(current);
  if(sysCurrent)
    removeTranslator(sysCurrent);

  // install new translator for the selected locale
  // when multiple translators are included, translations are fetched
  // in the reverse order. ie, last translator is searched first.
  // install system translations first
  sysCurrent = sysTranslators.value(locale, nullptr);
  if(sysCurrent)
    installTranslator(sysCurrent);
  // now install application translations
  current = translators.value(locale, nullptr);
  if(current)
    installTranslator(current);
}
