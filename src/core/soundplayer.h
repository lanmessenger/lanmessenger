// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include "uidefinitions.h"
#include "settings.h"

class lmcSoundPlayer {
public:
  lmcSoundPlayer(void);

  static bool isAvailable();
  static void play(const QString& filename);
  void play(SoundEvent event);
  void settingsChanged(void);

private:
  lmcSettings* pSettings;
  int eventState[SE_Max];
  QString sounds[SE_Max];
  bool playSound;
  bool noBusySound;
  bool noDNDSound;
};
