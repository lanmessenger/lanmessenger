/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
** 
** Contact:  dilipvrk@gmail.com
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


#ifndef DEFINITIONS_H
#define DEFINITIONS_H

//	Information about the application
#define IDA_TITLE		"LAN Messenger"
#ifdef Q_WS_WIN
#define IDA_PRODUCT		"LAN Messenger"
#define IDA_COMPANY		"LAN Messenger"
#else
#define IDA_PRODUCT		"lmc"
#define IDA_COMPANY		"lmc"
#endif
#define IDA_VERSION		"1.2.0"
#define IDA_DESCRIPTION	"LAN Messenger is a free peer-to-peer messaging application for intra-network communication "\
						"and does not require a server.\n"\
						"LAN Messenger works on essentially every popular desktop platform."
#define IDA_COPYRIGHT	"Copyright (C) 2010-2011 Dilip Radhakrishnan."
#define IDA_CONTACT		"dilipvrk@gmail.com"
#define IDA_DOMAIN		"http://lanmsngr.sourceforge.net"

#ifdef Q_WS_WIN
#define NULL			0
#endif

#define DELIMITER		"||"
#define DELIMITER_ESC	"\\|\\|"
#define APP_MARKER		"lmcmessage"

/****************************************************************************
**	Datagram type definitions
**	The enum and the string array should always be synced
****************************************************************************/
enum DatagramType {
	DT_None = 0,
	DT_Broadcast,
	DT_PublicKey,
	DT_Handshake,
	DT_Message,
	DT_Max
};

const QString DatagramTypeNames[] = {
	"",
	"BRDCST",
	"PUBKEY",
	"HNDSHK",
	"MESSAG"
};

/****************************************************************************
**	Message type definitions
**	The enum and the string array should always be synced
****************************************************************************/
enum MessageType {
	MT_Blank = 0,
	MT_Announce,
	MT_Depart,
	MT_UserData,
	MT_Broadcast,
	MT_Status,
	MT_Avatar,
	MT_UserName,
	MT_Ping,
	MT_Message,
	MT_GroupMessage,
	MT_File,
	MT_Acknowledge,
	MT_Failed,
	MT_Error,
	MT_OldVersion,
	MT_Query,
	MT_Info,
	MT_ChatState,
	MT_Group,
	//	These are used for local communication between layers
	MT_LocalFile,
	MT_LocalAvatar,
	MT_Refresh,
	MT_Max
};

const QString MessageTypeNames[] = {
	"",
	"announce",
	"depart",
	"userdata",
	"broadcast",
	"status",
	"avatar",
	"name",
	"ping",
	"message",
	"groupmessage",
	"file",
	"acknowledge",
	"failed",
	"error",
	"query",
	"info",
	"chatstate"
	"group",
	//	These are used for local communication between layers
	"localfile",
	"localavatar"
	"refresh"
};

enum FileMode {
	FM_Blank = 0,
	FM_Send,
	FM_Receive,
	FM_Max
};

const QString FileModeNames[] = {
	"",
	"send",
	"receive"
};

/****************************************************************************
**	File operation definitions
**	The enum and the string array should always be synced
****************************************************************************/
enum FileOp {
	FO_Blank = 0,
	FO_Request,
	FO_Accept,
	FO_Decline,
	FO_Cancel,
	FO_Progress,
	FO_Error,
	FO_Abort,
	FO_Complete,
	FO_Max
};

const QString FileOpNames[] = {
	"",
	"request",
	"accept",
	"decline",
	"cancel",
	"progress",
	"error",
	"abort",
	"complete"
};

/*
enum UserData {
	UD_Id = 0,
	UD_Name,
	UD_Address,
	UD_Version,
	UD_Status,
	UD_Avatar,
	UD_Logon,
	UD_Host,
	UD_OS,
	UD_FirstName,
	UD_LastName,
	UD_About,
	UD_Max
};

enum FileData {
	FD_Op = 0,
	FD_Id,
	FD_Path,
	FD_Name,
	FD_Size,
	//	Defined for versions > 1.1.58
	FD_Type,
	FD_Max
};
*/

/****************************************************************************
**	File operation definitions
**	The enum and the string array should always be synced
****************************************************************************/
enum FileType {
	FT_None = 0,
	FT_Normal,
	FT_Avatar,
	FT_Max
};

const QString FileTypeNames[] = {
	"",
	"normal",
	"avatar"
};

/****************************************************************************
**	Query operation definitions
**	The enum and the string array should always be synced
****************************************************************************/
enum QueryOp {
	QO_None = 0,
	QO_Get,
	QO_Result,
	QO_Max
};

const QString QueryOpNames[] = {
	"",
	"get",
	"result"
};

enum TrayMessageType { TM_Connection, TM_Status, TM_Transfer, TM_Minimize, TM_Max };
enum TrayMessageIcon { TMI_Info, TMI_Warning, TMI_Error, TMI_Max };

//	User status definitions
enum StatusType {
	StatusTypeOnline = 0,
	StatusTypeBusy,
	StatusTypeOffline,
	StatusTypeAway,
	StatusTypeMax
};

#define ST_COUNT	6
const QString statusCode[] = {"chat", "busy", "dnd", "brb", "away", "gone"};
const int statusType[] = {StatusTypeOnline, StatusTypeBusy, StatusTypeBusy, StatusTypeAway, StatusTypeAway, StatusTypeOffline};

#define GRP_DEFAULT		"General"

#endif // DEFINITIONS_H
