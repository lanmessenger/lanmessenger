/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
** 
** Contact:  dilipvradhakrishnan@gmail.com
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
#define IDA_VERSION		"1.1.58"
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

enum DatagramType {
	DT_Blank = 0,
	DT_Broadcast,
	DT_Handshake,
	DT_Message,
	DT_Max
};

enum MessageType {
	MT_Blank = 0,
	MT_Online,
	MT_Offline,
	MT_UserData,
	MT_UserDataAck,
	MT_Broadcast,
	MT_Status,
	MT_Avatar,
	MT_UserName,
	MT_Ping,
	MT_Message,
	MT_FileReq,
	MT_FileOp,
	MT_Acknowledge,
	MT_Failed,
	MT_Error,
	MT_OldVersion,
	MT_UserQuery,
	MT_UserInfo,
	MT_UserAction,
	MT_Group,
	//	These are used for local communication between layers
	MT_LocalFileReq,
	MT_LocalFileOp,
	MT_Refresh,
	MT_Max
};

enum FileMode {
	FM_Blank = 0,
	FM_Send,
	FM_Receive,
	FM_Max
};

enum FileOp {
	FO_Blank = 0,
	FO_Request,
	FO_Accept,
	FO_Decline,
	FO_CancelSend,
	FO_CancelReceive,
	FO_ProgressSend,
	FO_ProgressReceive,
	FO_ErrorSend,
	FO_ErrorReceive,
	FO_AbortSend,
	FO_AbortReceive,
	FO_CompleteSend,
	FO_CompleteReceive,
	FO_Max
};

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
	FD_Max
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
const QString statusCode[] = {"Available", "Busy", "NoDisturb", "RightBack", "Away", "Offline"};
const int statusType[] = {StatusTypeOnline, StatusTypeBusy, StatusTypeBusy, StatusTypeAway, StatusTypeAway, StatusTypeOffline};

#define GRP_DEFAULT		"General"

#endif // DEFINITIONS_H
