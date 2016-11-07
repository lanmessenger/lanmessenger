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


#ifndef CHATDEFINITIONS_H
#define CHATDEFINITIONS_H

#include "uidefinitions.h"

const int HTMLESC_COUNT = 5;
const QString htmlSymbol[] = {"&", "\"", "<", ">", " "};
const QString htmlEscape[] = {"&amp;", "&quot;", "&lt;", "&gt;", "&nbsp;"};

enum InfoType {
	IT_Ok			= 0x00,
	IT_Busy			= 0x01,
	IT_Offline		= 0x02,
	IT_Disconnected	= 0x04,
	IT_Away			= 0x08
};

/****************************************************************************
**	Chat state definitions
**	The enum and the string array should always be synced
****************************************************************************/
enum ChatState {
	CS_Blank = 0,
	CS_Active,
	CS_Composing,
	CS_Paused,
	CS_Inactive,
	CS_Gone,
	CS_Max
};

const QString ChatStateNames[] = {
	"",
	"active",
	"composing",
	"paused",
	"inactive",
	"gone"
};

#endif // CHATDEFINITIONS_H
