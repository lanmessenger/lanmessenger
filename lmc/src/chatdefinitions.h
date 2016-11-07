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


#ifndef CHATDEFINITIONS_H
#define CHATDEFINITIONS_H

#include "uidefinitions.h"

const int HTMLESC_COUNT = 4;
const QString htmlSymbol[] = {"&", "\"", "<", ">"};
const QString htmlEscape[] = {"&amp;", "&quot;", "&lt;", "&gt;"};

const QString htmlLineHeader(
	"<html>"\
	"<head>"\
	"<style type='text/css'>"\
		"body {font-family:'Arial';}"\
		"p{margin-top:0px;margin-bottom:0px;}"\
		"span.usr{font-weight:bold;}"\
		"span.msg{margin-left:18px;}"\
		"span.inf{color:rgb(96,96,96);}"\
		"span.fil{color:rgb(0,0,232);}"\
		"img.pic{float:left;}"\
		"tr.inf{background-color:rgb(234,234,255);}"\
		"tr.brd{background-color:rgb(248,252,214);}"\
	"</style>"\
	"</head>"\
	"<body>");
const QString htmlLineFooter("</body></html>");
const QString htmlHeaderGap(htmlLineHeader + 
	"<p><img width=1 height=8 src='' /></p><p><table width='100%' border='0' cellpadding='0' cellspacing='0'>");
const QString htmlHeaderCont(htmlLineHeader + 
	"<p><img width=1 height=2 src='' /></p><p><table width='100%' border='0' cellpadding='0' cellspacing='0'>");
const QString htmlFooter("</table></p>" + htmlLineFooter);

enum InfoType {
	IT_Ok			= 0x00,
	IT_Busy			= 0x01,
	IT_Offline		= 0x02,
	IT_Disconnected	= 0x04
};

class ChatHelper {
public:
	static void makeHtmlSafe(QString* lpszMessage) {
		for(int index = 0; index < HTMLESC_COUNT; index++)
			lpszMessage->replace(htmlSymbol[index], htmlEscape[index]);
	}

	static void encodeSmileys(QString* lpszMessage) {
		//	replace all emoticon images with corresponding text code
		for(int index = 0; index < SM_MAPCOUNT; index++) {
			QString code = smileyCode[index];
			makeHtmlSafe(&code);
			lpszMessage->replace("<img src=\"" + smileyPic[index] + "\" />", code);
		}
	}

	static void decodeSmileys(QString* lpszMessage) {
		//	replace text emoticons with corresponding images
		for(int index = 0; index < SM_MAPCOUNT; index++) {
			QString code = smileyCode[index];
			makeHtmlSafe(&code);
			lpszMessage->replace(code, "<img src='" + smileyPic[index] + "' />", Qt::CaseInsensitive);
		}
	}
};

#endif // CHATDEFINITIONS_H