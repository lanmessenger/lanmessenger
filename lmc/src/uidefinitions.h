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


#ifndef UIDEFINITIONS_H
#define UIDEFINITIONS_H

#include <QString>
#include "definitions.h"
#include "lmcstrings.h"

//	Resource name definitions
#define IDR_APPICON			":/icons/application"
#define IDR_CLOSE			":/images/close"
#define IDR_HISTORY			":/images/history"
#define IDR_INFO			":/images/info"
#define IDR_QUESTION		":/images/question"
#define IDR_CRITICAL		":/images/critical"
#define IDR_MESSENGER		":/images/messenger"
#define IDR_TOOLS			":/images/tools"
#define IDR_FILE			":/images/file"
#define IDR_TRANSFER		":/images/transfer"
#define IDR_FONT			":/images/font"
#define IDR_FONTCOLOR		":/images/fontcolor"
#define IDR_SMILEY			":/images/smiley"
#define IDR_SAVE			":/images/save"
#define IDR_PAINT			":/images/paint"
#define IDR_LOGO			":/images/logo"
#define IDR_LOGOSMALL		":/images/logosmall"
#define IDR_ACCEPT			":/images/accept"
#define IDR_DECLINE			IDR_CLOSE			//	same image as IDR_CLOSE
#define IDR_STOP			":/images/stop"
#define IDR_FOLDER			":/images/folder"
#define IDR_CRITICALMSG		":/images/criticalmsg"
#define IDR_BROADCASTMSG	":/images/broadcastmsg"
#define IDR_FILEMSG			":/images/filemsg"
#define IDR_CHATROOM		":/images/chatroom"
#define IDR_NEWCHATROOM		":/images/newchatroom"
#define IDR_ADDCONTACT		":/images/addcontact"
#define IDR_GENERALSET		":/images/systemsettings"
#define IDR_ACCOUNTSET		":/images/usersettings"
#define IDR_MESSAGESET		":/images/messagesettings"
#define IDR_HISTORYSET		":/images/historysettings"
#define IDR_ALERTSET		":/images/alertsettings"
#define IDR_NETWORKSET		":/images/networksettings"
#define IDR_TRANSFERSET		":/images/transfersettings"
#define IDR_THEMESET		":/images/themesettings"
#define IDR_HOTKEYSET		":/images/hotkeysettings"
#define IDR_REFRESH			":/images/refresh"
#define IDR_WEB				":/images/web"
#define IDR_PIXEL			":/images/pixel"
#define IDR_BLANK			":/images/blank"
#define IDR_CHAT			":/images/chat"
#define IDR_HGRIP			":/images/hgrip"
#define IDR_VGRIP			":/images/vgrip"
#define IDR_PLAY			":/images/play"
#define IDR_SENDFOLDER      ":/images/sendfolder"

#define IDR_LICENSETEXT		":/text/license"
#define IDR_THANKSTEXT		":/text/thanks"
#define IDR_LANG			":/lang"

//	item data role definitions
enum ItemDataRole {
	IdRole = Qt::UserRole + 1,	// Unique Id of the item
	TypeRole,	//	Whether item represents a Group or a User
	DataRole,	//	Custom data associated with the item
	StatusRole,	//	Status of the User
	AvatarRole,	//	Avatar image of the User
    SubtextRole,//	Subtext to be displayed on the item
    CapsRole    //  The capabilities of the user
};

//	font size definitions
#define FS_SMALL			0
#define FS_MEDIUM			1
#define FS_LARGE			2

#define FS_COUNT	3
//const QString fontSize[] = {"Small text", "Medium text", "Large text"};
const QString fontStyle[] = {"font-size:8.25pt;", "font-size:9pt;", "font-size:10.5pt;"};

//	status image definitions
#define IDR_AVAILABLE		":/images/status/online"
#define IDR_AWAY			":/images/status/away"
#define IDR_BUSY			":/images/status/busy"
#define IDR_NODISTURB		":/images/status/nodisturb"
#define IDR_OFFLINE			":/images/status/offline"
#define IDR_AVAILABLE_BBL	":/icons/bubbles/online"
#define IDR_AWAY_BBL		":/icons/bubbles/away"
#define IDR_BUSY_BBL		":/icons/bubbles/busy"
#define IDR_NODISTURB_BBL	":/icons/bubbles/nodisturb"
#define IDR_OFFLINE_BBL		":/icons/bubbles/offline"

//const QString statusDesc[] = {"Available", "Busy", "Do Not Disturb", "Be Right Back", "Away", "Appear Offline"};
const QString statusPic[] = {IDR_AVAILABLE, IDR_BUSY, IDR_NODISTURB, IDR_AWAY, IDR_AWAY, IDR_OFFLINE};
const QString bubblePic[] = {IDR_AVAILABLE_BBL, IDR_BUSY_BBL, IDR_NODISTURB_BBL, IDR_AWAY_BBL, IDR_AWAY_BBL, IDR_OFFLINE_BBL};

//	Smiley definitions
#define SMI_01			":/smileys/01"
#define SMI_02			":/smileys/02"
#define SMI_03			":/smileys/03"
#define SMI_04			":/smileys/04"
#define SMI_05			":/smileys/05"
#define SMI_06			":/smileys/06"
#define SMI_07			":/smileys/07"
#define SMI_08			":/smileys/08"
#define SMI_09			":/smileys/09"
#define SMI_10			":/smileys/10"
#define SMI_11			":/smileys/11"
#define SMI_12			":/smileys/12"
#define SMI_13			":/smileys/13"
#define SMI_14			":/smileys/14"
#define SMI_15			":/smileys/15"
#define SMI_16			":/smileys/16"
#define SMI_17			":/smileys/17"
#define SMI_18			":/smileys/18"
#define SMI_19			":/smileys/19"
#define SMI_20			":/smileys/20"
#define SMI_21			":/smileys/21"
#define SMI_22			":/smileys/22"
#define SMI_23			":/smileys/23"
#define SMI_24			":/smileys/24"
#define SMI_25			":/smileys/25"
#define SMI_26			":/smileys/26"
#define SMI_27			":/smileys/27"
#define SMI_28			":/smileys/28"
#define SMI_29			":/smileys/29"
#define SMI_30			":/smileys/30"
#define SMI_31			":/smileys/31"
#define SMI_32			":/smileys/32"
#define SMI_33			":/smileys/33"
#define SMI_34			":/smileys/34"
#define SMI_35			":/smileys/35"
#define SMI_36			":/smileys/36"
#define SMI_37			":/smileys/37"
#define SMI_38			":/smileys/38"
#define SMI_39			":/smileys/39"
#define SMI_40			":/smileys/40"
#define SMI_41			":/smileys/41"
#define SMI_42			":/smileys/42"
#define SMI_43			":/smileys/43"
#define SMI_44			":/smileys/44"
#define SMI_45			":/smileys/45"
#define SMI_46			":/smileys/46"
#define SMI_47			":/smileys/47"
#define SMI_48			":/smileys/48"
#define SMI_49			":/smileys/49"
#define SMI_50			":/smileys/50"
#define SMI_51			":/smileys/51"
#define SMI_52			":/smileys/52"
#define SMI_53			":/smileys/53"
#define SMI_54			":/smileys/54"
#define SMI_55			":/smileys/55"
#define SMI_56			":/smileys/56"
#define SMI_57			":/smileys/57"
#define SMI_58			":/smileys/58"
#define SMI_59			":/smileys/59"
#define SMI_60			":/smileys/60"
#define SMI_61			":/smileys/61"
#define SMI_62			":/smileys/62"
#define SMI_63			":/smileys/63"
#define SMI_64			":/smileys/64"
#define SMI_65			":/smileys/65"
#define SMI_66			":/smileys/66"
#define SMI_67			":/smileys/67"
#define SMI_68			":/smileys/68"
#define SMI_69			":/smileys/69"
#define SMI_70			":/smileys/70"

#define SM_COUNT		70
#define SM_MAPCOUNT		81
const QString smileyCode[] = {
	":-)", ":-D", ";-)", ":-O", ":-P", "(H)", ":-@", ":-S", ":-$", ":-(", 
	":'(", ":-|", "(ST)", "8o|", "8-|", "+o(", "<:o)", "|-)", "*-)", ":-#", 
	":-*", "^o)", "8-)", "(L)", "(U)", "(D)", "(B)", "(@)", "(&)", "(SN)", 
	"(BAH)", "(S)", "(*)", "(#)", "(R)", "({)", "(})", "(K)", "(F)", "(W)", 
	"(O)", "(G)", "(^)", "(P)", "(I)", "(C)", "(T)", "(MP)", "(AU)", "(AP)", 
	"(CO)", "(MO)", "(~)", "(8)", "(PI)", "(SO)", "(E)", "(Z)", "(X)", "(IP)", 
	"(UM)", "(OK)", "(SS)", "(Y)", "(N)", "(YN)", "(H5)", "(TU)", ":-[", "(M)",
	":)", ":D", ";)", ":O", ":P", ":@", ":S", ":$", ":(", ":|", ":["};
const QString smileyPic[] = {
	SMI_01, SMI_02, SMI_03, SMI_04, SMI_05, SMI_06, SMI_07, SMI_08, SMI_09, SMI_10,
	SMI_11, SMI_12, SMI_13, SMI_14, SMI_15, SMI_16, SMI_17, SMI_18, SMI_19, SMI_20,
	SMI_21, SMI_22, SMI_23, SMI_24, SMI_25, SMI_26, SMI_27, SMI_28, SMI_29, SMI_30,
	SMI_31, SMI_32, SMI_33, SMI_34, SMI_35, SMI_36, SMI_37, SMI_38, SMI_39, SMI_40, 
	SMI_41, SMI_42, SMI_43, SMI_44, SMI_45, SMI_46, SMI_47, SMI_48, SMI_49, SMI_50, 
	SMI_51, SMI_52, SMI_53, SMI_54, SMI_55, SMI_56, SMI_57, SMI_58, SMI_59, SMI_60, 
	SMI_61, SMI_62, SMI_63, SMI_64, SMI_65, SMI_66, SMI_67, SMI_68, SMI_69, SMI_70,
	SMI_01, SMI_02, SMI_03, SMI_04, SMI_05, SMI_07, SMI_08, SMI_09, SMI_10, SMI_12, SMI_70};

//	Avatar definitions
#define AVT_DEFAULT			":/avatars/default"
#define AVT_0				":/avatars/0"
#define AVT_1				":/avatars/1"
#define AVT_2				":/avatars/2"
#define AVT_3				":/avatars/3"
#define AVT_4				":/avatars/4"
#define AVT_5				":/avatars/5"
#define AVT_6				":/avatars/6"
#define AVT_7				":/avatars/7"
#define AVT_8				":/avatars/8"
#define AVT_9				":/avatars/9"
#define AVT_10				":/avatars/10"
#define AVT_11				":/avatars/11"
#define AVT_12				":/avatars/12"
#define AVT_13				":/avatars/13"
#define AVT_14				":/avatars/14"
#define AVT_15				":/avatars/15"
#define AVT_16				":/avatars/16"
#define AVT_17				":/avatars/17"
#define AVT_18				":/avatars/18"
#define AVT_19				":/avatars/19"

#define AVT_COUNT	20
const QString avtPic[] = {AVT_0, AVT_1, AVT_2, AVT_3, AVT_4, AVT_5, AVT_6, AVT_7, AVT_8, AVT_9, 
	AVT_10, AVT_11, AVT_12, AVT_13, AVT_14, AVT_15, AVT_16, AVT_17, AVT_18, AVT_19};

#define AVT_WIDTH		48
#define AVT_HEIGHT		48

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

#define SND_NEWMESSAGE		"./sounds/newmessage.wav"
#define SND_USERONLINE		"./sounds/useronline.wav"
#define SND_USEROFFLINE		"./sounds/useroffline.wav"
#define SND_NEWFILE			"./sounds/newfile.wav"
#define SND_FILEDONE		"./sounds/filedone.wav"
#define SND_NEWPUBMESSAGE	SND_NEWMESSAGE

#define SE_COUNT	6
const QString soundFile[] = {SND_NEWMESSAGE, SND_USERONLINE, SND_USEROFFLINE, SND_NEWFILE, SND_FILEDONE, SND_NEWPUBMESSAGE};

#define AT_COUNT	8
const int awayTimeVal[] = {5, 10, 15, 20, 30, 45, 60, 0};

//	User list views
enum UserListView {
	ULV_Detailed = 0,
	ULV_Compact,
	ULV_Max
};

#define ULV_COUNT	2
const int itemViewHeight[] = {36, 20};

#define RTL_LAYOUT			"RTL"

#ifdef Q_OS_MAC
#define GRAY_TEXT_COLOR     QApplication::palette().color(QPalette::Shadow).darker(175)
#else
#define GRAY_TEXT_COLOR     QApplication::palette().color(QPalette::Shadow)
#endif

#endif // UIDEFINITIONS_H
