// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QtGlobal>
#ifdef Q_OS_WIN
#include <Windows.h>
#include <QLibrary>
#else
#include <QSoundEffect>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QUrl>
#endif
#include "soundplayer.h"
#include "settings.h"
#include "stdlocation.h"
#include <QFile>
#include <QThread>
#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_WIN
typedef BOOL(WINAPI* sndPlaySoundFunc)(LPCSTR lpszSound, UINT fuSound);
static sndPlaySoundFunc sndPlaySoundFromDll = nullptr;
#endif

#ifndef Q_OS_WIN
// Zero-initialized: no effect bound until updateSources()/syncBinding() runs.
QSoundEffect* lmcSoundPlayer::effects[SE_Max] = {};
#endif
QString lmcSoundPlayer::sounds[SE_Max];
int lmcSoundPlayer::eventState[SE_Max];
bool lmcSoundPlayer::playSound = false;
bool lmcSoundPlayer::noBusySound = false;
bool lmcSoundPlayer::noDNDSound = false;

namespace {
// QSoundEffect is not thread-safe and all of the pool below lives on the GUI
// thread. Q_ASSERTs catch violations in debug builds; this guard keeps release
// builds from silent UB (loud skip instead of a crash later).
bool onGuiThread(const char* where) {
  QCoreApplication* app = QCoreApplication::instance();
  if(app != nullptr && QThread::currentThread() == app->thread())
    return true;
  qWarning("%s: called outside the GUI thread, ignored", where);
  return false;
}
} // namespace

lmcSoundPlayer::lmcSoundPlayer(void) {
  pSettings = new lmcSettings();
  settingsChanged();
#ifdef Q_OS_WIN
  if(sndPlaySoundFromDll == nullptr) {
    QLibrary winmmDll("winmm");
    sndPlaySoundFromDll = (sndPlaySoundFunc)winmmDll.resolve("sndPlaySoundA");
  }
#endif
}

bool lmcSoundPlayer::isAvailable() {
#ifdef Q_OS_WIN
  return sndPlaySoundFromDll != nullptr;
#else
  return !QMediaDevices::audioOutputs().isEmpty();
#endif
}

void lmcSoundPlayer::play(const QString& filename) {
  Q_ASSERT(QCoreApplication::instance() != nullptr &&
           QThread::currentThread() == QCoreApplication::instance()->thread());
  if(!onGuiThread("lmcSoundPlayer::play"))
    return;
#ifdef Q_OS_WIN
  if(sndPlaySoundFromDll)
    sndPlaySoundFromDll(filename.toStdString().c_str(), SND_ASYNC);
#else
  // Own instance, never shared with event effects: previewing a sound must
  // not switch the source of an effect that event playback relies on.
  // Leaked on purpose: a static QSoundEffect would be destroyed after
  // QApplication teardown, and its destructor posts events with no event
  // loop left, aborting the process on Qt 6.8.
  static QSoundEffect* previewEffect = new QSoundEffect();
  previewEffect->setSource(QUrl::fromLocalFile(filename));
  previewEffect->play();
#endif
}

#ifndef Q_OS_WIN
QString lmcSoundPlayer::resolveEventFile(int index) {
  if(index < 0 || index >= SE_Max)
    return QString();
  // Missing stored file falls back to the default.
  QString file = StdLocation::resolveSoundFile(sounds[index]);
  if(!QFile::exists(file))
    file = StdLocation::resolveSoundFile(soundFile[index]);
  return QFile::exists(file) ? file : QString();
}

void lmcSoundPlayer::updateSources(void) {
  Q_ASSERT(QCoreApplication::instance() != nullptr &&
           QThread::currentThread() == QCoreApplication::instance()->thread());
  for(int index = 0; index < SE_Max; index++)
    syncBinding(index);
}

void lmcSoundPlayer::syncBinding(int index) {
  if(!onGuiThread("lmcSoundPlayer::syncBinding"))
    return;
  if(index < 0 || index >= SE_Max)
    return;
  QString file = resolveEventFile(index);
  const QUrl source = file.isEmpty() ? QUrl() : QUrl::fromLocalFile(file);
  QSoundEffect* effect = effects[index];
  if(effect == nullptr) {
    if(source.isEmpty())
      return;
    // Leaked on purpose, see play() above.
    effect = new QSoundEffect();
    effects[index] = effect;
  }
  if(effect->source() != source)
    effect->setSource(source);
}
#endif

void lmcSoundPlayer::play(SoundEvent event) {
  Q_ASSERT(QCoreApplication::instance() != nullptr &&
           QThread::currentThread() == QCoreApplication::instance()->thread());
  QString localStatus = pSettings->value(IDS_STATUS, IDS_STATUS_VAL).toString();
  if(!playSound || (localStatus == "Busy" && noBusySound) || (localStatus == "NoDisturb" && noDNDSound))
    return;

  if(event < 0 || event >= SE_Max || !eventState[event])
    return;

#ifdef Q_OS_WIN
  // Missing stored file falls back to the default.
  QString file = StdLocation::resolveSoundFile(sounds[event]);
  if(!QFile::exists(file))
    file = StdLocation::resolveSoundFile(soundFile[event]);
  play(file);
#else
  // QSoundEffect::play() does not block; only (re)binding the source can.
  // syncBinding() re-resolves cheaply on every call (local stat, no backend
  // contact) and rebinds solely on path change; same-path rewrites are not
  // picked up until settings change.
  syncBinding(event);
  QSoundEffect* effect = effects[event];
  if(effect != nullptr && !effect->source().isEmpty())
    effect->play();
#endif
}

void lmcSoundPlayer::settingsChanged(void) {
  // Reset to defaults first: a removed array group (Reset sounds) reads back
  // as zero iterations, which must mean "everything default", not "keep stale".
  for(int index = 0; index < SE_Max; index++) {
    eventState[index] = Qt::Checked;
    sounds[index] = soundFile[index];
  }
  int size = qMin(pSettings->beginReadArray(IDS_SOUNDEVENTHDR), (int)SE_Max);
  for(int index = 0; index < size; index++) {
    pSettings->setArrayIndex(index);
    eventState[index] = pSettings->value(IDS_SOUNDEVENT, IDS_SOUNDEVENT_VAL).toInt();
  }
  pSettings->endArray();

  size = qMin(pSettings->beginReadArray(IDS_SOUNDFILEHDR), (int)SE_Max);
  for(int index = 0; index < size; index++) {
    pSettings->setArrayIndex(index);
    // Legacy "./sounds/..." normalizes to basename on load.
    sounds[index] = StdLocation::normalizeSoundFile(
      pSettings->value(IDS_SOUNDFILE, soundFile[index]).toString());
  }
  pSettings->endArray();

  playSound = pSettings->value(IDS_SOUND, IDS_SOUND_VAL).toBool();
  noBusySound = pSettings->value(IDS_NOBUSYSOUND, IDS_NOBUSYSOUND_VAL).toBool();
  noDNDSound = pSettings->value(IDS_NODNDSOUND, IDS_NODNDSOUND_VAL).toBool();
#ifndef Q_OS_WIN
  updateSources();
#endif
}
