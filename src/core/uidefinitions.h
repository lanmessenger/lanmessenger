// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QString>
#include "definitions.h"
#include "lmcstrings.h"

//	Resource name definitions
#define IDR_APPICON ":/icons/application"
#define IDR_CLOSE ":/images/close"
#define IDR_HISTORY ":/images/history"
#define IDR_INFO ":/images/info"
#define IDR_QUESTION ":/images/question"
#define IDR_CRITICAL ":/images/critical"
#define IDR_MESSENGER ":/images/messenger"
#define IDR_TOOLS ":/images/tools"
#define IDR_FILE ":/images/file"
#define IDR_TRANSFER ":/images/transfer"
#define IDR_FONT ":/images/font"
#define IDR_FONTCOLOR ":/images/fontcolor"
#define IDR_SMILEY ":/images/smiley"
#define IDR_SAVE ":/images/save"
#define IDR_PAINT ":/images/paint"
#define IDR_LOGO ":/images/logo"
#define IDR_LOGOSMALL ":/images/logosmall"
#define IDR_ACCEPT ":/images/accept"
#define IDR_DECLINE IDR_CLOSE //	same image as IDR_CLOSE
#define IDR_STOP ":/images/stop"
#define IDR_FOLDER ":/images/folder"
#define IDR_CRITICALMSG ":/images/criticalmsg"
#define IDR_BROADCASTMSG ":/images/broadcastmsg"
#define IDR_FILEMSG ":/images/filemsg"
#define IDR_CHATROOM ":/images/chatroom"
#define IDR_NEWCHATROOM ":/images/newchatroom"
#define IDR_ADDCONTACT ":/images/addcontact"
#define IDR_GENERALSET ":/images/systemsettings"
#define IDR_ACCOUNTSET ":/images/usersettings"
#define IDR_MESSAGESET ":/images/messagesettings"
#define IDR_HISTORYSET ":/images/historysettings"
#define IDR_ALERTSET ":/images/alertsettings"
#define IDR_NETWORKSET ":/images/networksettings"
#define IDR_TRANSFERSET ":/images/transfersettings"
#define IDR_THEMESET ":/images/themesettings"
#define IDR_HOTKEYSET ":/images/hotkeysettings"
#define IDR_REFRESH ":/images/refresh"
#define IDR_WEB ":/images/web"
#define IDR_PIXEL ":/images/pixel"
#define IDR_CHAT ":/images/chat"
#define IDR_HGRIP ":/images/hgrip"
#define IDR_VGRIP ":/images/vgrip"
#define IDR_PLAY ":/images/play"
#define IDR_SENDFOLDER ":/images/sendfolder"

#define IDR_LICENSETEXT ":/text/license"
#define IDR_THANKSTEXT ":/text/thanks"
#define IDR_LANG ":/lang"

//	item data role definitions
enum ItemDataRole {
  IdRole = Qt::UserRole + 1, // Unique Id of the item
  TypeRole,                  //	Whether item represents a Group or a User
  DataRole,                  //	Custom data associated with the item
  StatusRole,                //	Status of the User
  AvatarRole,                //	Avatar image of the User
  SubtextRole,               //	Subtext to be displayed on the item
  CapsRole                   //  The capabilities of the user
};

//	font size definitions
#define FS_SMALL 0
#define FS_MEDIUM 1
#define FS_LARGE 2

#define FS_COUNT 3
//const QString fontSize[] = {"Small text", "Medium text", "Large text"};
const QString fontStyle[] = {"font-size:8.25pt;", "font-size:9pt;", "font-size:10.5pt;"};

//	status image definitions
#define IDR_AVAILABLE ":/images/status/online"
#define IDR_AWAY ":/images/status/away"
#define IDR_BUSY ":/images/status/busy"
#define IDR_NODISTURB ":/images/status/nodisturb"
#define IDR_OFFLINE ":/images/status/offline"
#define IDR_AVAILABLE_BBL ":/icons/bubbles/online"
#define IDR_AWAY_BBL ":/icons/bubbles/away"
#define IDR_BUSY_BBL ":/icons/bubbles/busy"
#define IDR_NODISTURB_BBL ":/icons/bubbles/nodisturb"
#define IDR_OFFLINE_BBL ":/icons/bubbles/offline"

//const QString statusDesc[] = {"Available", "Busy", "Do Not Disturb", "Be Right Back", "Away", "Appear Offline"};
const QString statusPic[] = {IDR_AVAILABLE, IDR_BUSY, IDR_NODISTURB, IDR_AWAY, IDR_AWAY, IDR_OFFLINE};
const QString bubblePic[] = {IDR_AVAILABLE_BBL, IDR_BUSY_BBL, IDR_NODISTURB_BBL, IDR_AWAY_BBL, IDR_AWAY_BBL, IDR_OFFLINE_BBL};

//	Avatar definitions
#define AVT_DEFAULT ":/avatars/default"
#define AVT_0 ":/avatars/0"
#define AVT_1 ":/avatars/1"
#define AVT_2 ":/avatars/2"
#define AVT_3 ":/avatars/3"
#define AVT_4 ":/avatars/4"
#define AVT_5 ":/avatars/5"
#define AVT_6 ":/avatars/6"
#define AVT_7 ":/avatars/7"
#define AVT_8 ":/avatars/8"
#define AVT_9 ":/avatars/9"
#define AVT_10 ":/avatars/10"
#define AVT_11 ":/avatars/11"
#define AVT_12 ":/avatars/12"
#define AVT_13 ":/avatars/13"
#define AVT_14 ":/avatars/14"
#define AVT_15 ":/avatars/15"
#define AVT_16 ":/avatars/16"
#define AVT_17 ":/avatars/17"
#define AVT_18 ":/avatars/18"
#define AVT_19 ":/avatars/19"

#define AVT_COUNT 20
const QString avtPic[] = {AVT_0, AVT_1, AVT_2, AVT_3, AVT_4, AVT_5, AVT_6, AVT_7, AVT_8, AVT_9,
                          AVT_10, AVT_11, AVT_12, AVT_13, AVT_14, AVT_15, AVT_16, AVT_17, AVT_18, AVT_19};

#define AVT_WIDTH 48
#define AVT_HEIGHT 48

//	Sound events definitions
enum SoundEvent {
  SE_NewMessage = 0,
  SE_UserOnline,
  SE_UserOffline,
  SE_NewFile,
  SE_FileDone,
  SE_NewPubMessage,
  SE_Max
};

#define SND_NEWMESSAGE "./sounds/newmessage.wav"
#define SND_USERONLINE "./sounds/useronline.wav"
#define SND_USEROFFLINE "./sounds/useroffline.wav"
#define SND_NEWFILE "./sounds/newfile.wav"
#define SND_FILEDONE "./sounds/filedone.wav"
#define SND_NEWPUBMESSAGE SND_NEWMESSAGE

#define SE_COUNT 6
const QString soundFile[] = {SND_NEWMESSAGE, SND_USERONLINE, SND_USEROFFLINE, SND_NEWFILE, SND_FILEDONE, SND_NEWPUBMESSAGE};

#define AT_COUNT 8
const int awayTimeVal[] = {5, 10, 15, 20, 30, 45, 60, 0};

//	User list views
enum UserListView {
  ULV_Detailed = 0,
  ULV_Compact,
  ULV_Max
};

#define ULV_COUNT 2
const int itemViewHeight[] = {36, 20};

#define RTL_LAYOUT "RTL"

#ifdef Q_OS_MAC
#define GRAY_TEXT_COLOR QApplication::palette().color(QPalette::Shadow).darker(175)
#else
#define GRAY_TEXT_COLOR QApplication::palette().color(QPalette::Shadow)
#endif
