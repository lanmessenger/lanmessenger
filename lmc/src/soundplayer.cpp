/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
** 
** Contact:  qualiatech@gmail.com
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/


#include "soundplayer.h"

lmcSoundPlayer::lmcSoundPlayer(void) {
	pSettings = new lmcSettings();
	for(int index = 0; index < SE_Max; index++) {
		eventState[index] = Qt::Checked;
	}
	settingsChanged();
}

void lmcSoundPlayer::play(SoundEvent event) {
	QString localStatus = pSettings->value(IDS_STATUS, IDS_STATUS_VAL).toString();
	if(!playSound || (localStatus == "Busy" && noBusySound) || (localStatus == "NoDisturb" && noDNDSound))
		return;

	if(!eventState[event])
		return;

	QSound::play(soundFile[event]);
}

void lmcSoundPlayer::settingsChanged(void) {
	int size = pSettings->beginReadArray(IDS_SOUNDEVENTHDR);
	for(int index = 0; index < size; index++) {
		pSettings->setArrayIndex(index);
		eventState[index] = pSettings->value(IDS_EVENT).toInt();
	}
	pSettings->endArray();

	playSound = pSettings->value(IDS_SOUND, IDS_SOUND_VAL).toBool();
	noBusySound = pSettings->value(IDS_NOBUSYSOUND, IDS_NOBUSYSOUND_VAL).toBool();
	noDNDSound = pSettings->value(IDS_NODNDSOUND, IDS_NODNDSOUND_VAL).toBool();
}
