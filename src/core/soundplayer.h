// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include "uidefinitions.h"

class lmcSettings;
#ifndef Q_OS_WIN
class QSoundEffect;
#endif

class lmcSoundPlayer {
public:
  lmcSoundPlayer(void);

  static bool isAvailable();
  static void play(const QString& filename);
  void play(SoundEvent event);
  void settingsChanged(void);

private:
#ifndef Q_OS_WIN
  // Binds every event eagerly (ctor, settingsChanged): fail-fast here, in the
  // user's presence, instead of hanging on the first incoming message later.
  void updateSources(void);
  // Binds one event, but touches the backend only if the resolved file
  // changed since the last bind: setSource() restarts the pulse stream and
  // can block forever on a wedged backend, so a steady hot path never pays it.
  void syncBinding(int index);
  QString resolveEventFile(int index);
  // App-wide pool: one effect per event, shared by all instances (same
  // settings store; per-window sounds would need per-instance binding).
  // GUI thread only. Leaked on purpose, never deleted (see .cpp).
  static QSoundEffect* effects[SE_Max];
#endif
  lmcSettings* pSettings;
  // Shared config cache (single settings store for all instances), so a stale
  // instance can never rebind the shared pool or decide by outdated flags.
  static int eventState[SE_Max];
  static QString sounds[SE_Max];
  static bool playSound;
  static bool noBusySound;
  static bool noDNDSound;
};
