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


#include <QMenu>
#include <QAction>
#include "messagelog.h"

lmcMessageLog::lmcMessageLog(QWidget *parent) : QWebView(parent) {
	connect(this, SIGNAL(linkClicked(QUrl)), this, SLOT(log_linkClicked(QUrl)));
	connect(this->page()->mainFrame(), SIGNAL(contentsSizeChanged(QSize)),
			this, SLOT(log_contentsSizeChanged(QSize)));
	connect(this->page(), SIGNAL(linkHovered(QString, QString, QString)),
			this, SLOT(log_linkHovered(QString, QString, QString)));

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	setRenderHints(QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	QAction* action = pageAction(QWebPage::Copy);
	action->setShortcut(QKeySequence::Copy);
	addAction(action);
	action = pageAction(QWebPage::CopyLinkToClipboard);
	addAction(action);
	action = pageAction(QWebPage::SelectAll);
	action->setShortcut(QKeySequence::SelectAll);
	addAction(action);

	contextMenu = new QMenu(this);
	copyAction = contextMenu->addAction("&Copy", this, SLOT(copyAction_triggered()), QKeySequence::Copy);
	copyLinkAction = contextMenu->addAction("&Copy Link", this, SLOT(copyLinkAction_triggered()));
	contextMenu->addSeparator();
	selectAllAction = contextMenu->addAction("Select &All", this,
							SLOT(selectAllAction_triggered()), QKeySequence::SelectAll);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
	setContextMenuPolicy(Qt::CustomContextMenu);

	participantAvatars.clear();
	hasData = false;
	messageTime = false;
	messageDate = false;
	allowLinks = false;
	pathToLink = false;
	trimMessage = true;
	fontSizeVal = 0;
	sendFileMap.clear();
	receiveFileMap.clear();
	lastId = QString::null;
	messageLog.clear();
	linkHovered = false;
	outStyle = false;
}

lmcMessageLog::~lmcMessageLog() {
}

void lmcMessageLog::initMessageLog(QString themePath, bool clearLog) {
	if(clearLog)
		messageLog.clear();
	lastId = QString::null;
	this->themePath = themePath;
	themeData = lmcTheme::loadTheme(themePath);
	setHtml(themeData.document);
}

void lmcMessageLog::appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage,
		bool bReload) {

	if(!pMessage && type != MT_Error)
		return;

	QString message;
	QString html;
	QString caption;
	QDateTime time;
	QFont font;
	QColor color;
	QString fontStyle;
	QString id = QString::null;
	bool addToLog = true;

	removeMessageLog("_lmc_statediv");

	switch(type) {
	case MT_Message:
		time.setMSecsSinceEpoch(pMessage->header(XN_TIME).toLongLong());
		message = pMessage->data(XN_MESSAGE);
		font.fromString(pMessage->data(XN_FONT));
		color.setNamedColor(pMessage->data(XN_COLOR));
		appendMessage(lpszUserId, lpszUserName, &message, &time, &font, &color);
		lastId = *lpszUserId;
		break;
	case MT_PublicMessage:
	case MT_GroupMessage:
		time.setMSecsSinceEpoch(pMessage->header(XN_TIME).toLongLong());
		message = pMessage->data(XN_MESSAGE);
		font.fromString(pMessage->data(XN_FONT));
		color.setNamedColor(pMessage->data(XN_COLOR));
		appendPublicMessage(lpszUserId, lpszUserName, &message, &time, &font, &color);
		lastId = *lpszUserId;
		break;
	case MT_Broadcast:
		time.setMSecsSinceEpoch(pMessage->header(XN_TIME).toLongLong());
		message = pMessage->data(XN_BROADCAST);
		appendBroadcast(lpszUserId, lpszUserName, &message, &time);
		lastId  = QString::null;
		break;
	case MT_ChatState:
		message = pMessage->data(XN_CHATSTATE);
		caption = getChatStateMessage((ChatState)Helper::indexOf(ChatStateNames, CS_Max, message));
		if(!caption.isNull()) {
			html = themeData.stateMsg;
			html.replace("%iconpath%", "qrc"IDR_BLANK);
			html.replace("%sender%", caption.arg(*lpszUserName));
			html.replace("%message%", "");
			appendMessageLog(&html);
		}
		addToLog = false;
		break;
	case MT_Failed:
		message = pMessage->data(XN_MESSAGE);
		font.fromString(pMessage->data(XN_FONT));
		color.setNamedColor(pMessage->data(XN_COLOR));
		html = themeData.sysMsg;
		caption = tr("This message was not delivered to %1:");
		fontStyle = getFontStyle(&font, &color, true);
		decodeMessage(&message);
		html.replace("%iconpath%", "qrc"IDR_CRITICALMSG);
		html.replace("%sender%", caption.arg(*lpszUserName));
		html.replace("%style%", fontStyle);
		html.replace("%message%", message);
		appendMessageLog(&html);
		lastId  = QString::null;
		break;
	case MT_Error:
		html = themeData.sysMsg;
		html.replace("%iconpath%", "qrc"IDR_CRITICALMSG);
		html.replace("%sender%", tr("Your message was not sent."));
		html.replace("%message%", "");
		appendMessageLog(&html);
		lastId  = QString::null;
		addToLog = false;
		break;
	case MT_File:
	case MT_LocalFile:
		appendFileMessage(type, lpszUserName, pMessage, bReload);
		id = pMessage->data(XN_TEMPID);
		pMessage->removeData(XN_TEMPID);
		lastId = QString::null;
		break;
	case MT_Join:
	case MT_Leave:
		message = pMessage->data(XN_GROUPMSGOP);
		caption = getChatRoomMessage((GroupMsgOp)Helper::indexOf(GroupMsgOpNames, GMO_Max, message));
		if(!caption.isNull()) {
			html = themeData.sysMsg;
			html.replace("%iconpath%", "qrc"IDR_BLANK);
			html.replace("%sender%", caption.arg(*lpszUserName));
			html.replace("%message%", "");
			appendMessageLog(&html);
		}
		lastId = QString::null;
	default:
		break;
	}

	if(!bReload && addToLog && pMessage) {
		XmlMessage xmlMessage = pMessage->clone();
		QString userId = lpszUserId ? *lpszUserId : QString::null;
		QString userName = lpszUserName ? *lpszUserName : QString::null;
		messageLog.append(SingleMessage(type, userId, userName, xmlMessage, id));
	}
}

void lmcMessageLog::updateFileMessage(FileMode mode, FileOp op, QString fileId) {
	QString szMessage = getFileStatusMessage(mode, op);
	QWebFrame* frame = page()->mainFrame();
	QWebElement document = frame->documentElement();
	QWebElement body = document.findFirst("body");
	QString selector = "span#";
	QString tempId = (mode == FM_Send) ? "send" : "receive";
	tempId.append(fileId);
	selector.append(tempId);
	QWebElement span = body.findFirst(selector);
	span.setPlainText(szMessage);

	//	update the entry in message log
	for(int index = 0; index < messageLog.count(); index++) {
		SingleMessage msg = messageLog.at(index);
		if(tempId.compare(msg.id) == 0) {
			XmlMessage xmlMessage = msg.message;
			xmlMessage.removeData(XN_FILEOP);
			xmlMessage.addData(XN_FILEOP, FileOpNames[op]);
			msg.message = xmlMessage;
			break;
		}
	}
}

void lmcMessageLog::updateUserName(QString* lpszUserId, QString* lpszUserName) {
	//	update the entries in message log
	for(int index = 0; index < messageLog.count(); index++) {
		SingleMessage msg = messageLog.takeAt(index);
		if(lpszUserId->compare(msg.userId) == 0)
			msg.userName = *lpszUserName;
		messageLog.insert(index, msg);
	}

	reloadMessageLog();
}

void lmcMessageLog::updateAvatar(QString* lpszUserId, QString* lpszFilePath) {
	participantAvatars.insert(*lpszUserId, *lpszFilePath);

	reloadMessageLog();
}

void lmcMessageLog::reloadMessageLog(void) {
	initMessageLog(themePath, false);
	for(int index = 0; index < messageLog.count(); index++) {
		SingleMessage msg = messageLog[index];
		appendMessageLog(msg.type, &msg.userId, &msg.userName, &msg.message, true);
	}
}

QString lmcMessageLog::prepareMessageLogForSave(OutputFormat format) {
	QDateTime time;

	if(format == HtmlFormat) {
		QString html =
			"<html><head><style type='text/css'>"\
			"*{font-size: 9pt;} span.salutation {float:left; font-weight: bold;}"\
			"span.time {float: right;} span.message {clear: both; display: block;}"\
			"p {border-bottom: 1px solid #CCC;}"\
			"</style></head><body>";

		for(int index = 0; index < messageLog.count(); index++) {
			SingleMessage msg = messageLog.at(index);
			if(msg.type == MT_Message || msg.type == MT_GroupMessage) {
				time.setMSecsSinceEpoch(msg.message.header(XN_TIME).toLongLong());
				QString htmlMsg =
					"<p><span class='salutation'>" + msg.userName + ":</span>"\
					"<span class='time'>" + time.time().toString(Qt::SystemLocaleShortDate) + "</span>"\
					"<span class='message'>" + msg.message.data(XN_MESSAGE) + "</span></p>";
				html.append(htmlMsg);
			}
		}

		html.append("</body></html>");
		return html;
	} else {
		QString text;
		for(int index = 0; index < messageLog.count(); index++) {
			SingleMessage msg = messageLog.at(index);
			if(msg.type == MT_Message || msg.type == MT_GroupMessage) {
				time.setMSecsSinceEpoch(msg.message.header(XN_TIME).toLongLong());
				QString textMsg =
					msg.userName + " [" + time.time().toString(Qt::SystemLocaleShortDate) + "]:\n" +
					msg.message.data(XN_MESSAGE) + "\n\n";
				text.append(textMsg);
			}
		}

		return text;
	}
}

void lmcMessageLog::changeEvent(QEvent* event) {
	switch(event->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
	default:
		break;
	}

	QWidget::changeEvent(event);
}

void lmcMessageLog::log_linkClicked(QUrl url) {
	QString linkPath = url.toString();

	//	this is a hack so qdesktopservices can open a network path
	if(linkPath.startsWith("file")) {
		// strip out the 'file:' prefix and get the path
		linkPath = linkPath.mid(5);
		// use a url that contains native separators
		QDesktopServices::openUrl(QUrl(QDir::toNativeSeparators(linkPath)));
		return;
	} else if(linkPath.startsWith("www")) {
		// prepend 'http://' to link
		linkPath.prepend("http://");
		QDesktopServices::openUrl(QUrl(linkPath));
		return;
	} else if(!linkPath.startsWith("lmc")) {
		QDesktopServices::openUrl(url);
		return;
	}

	QStringList linkData = linkPath.split("/", QString::SkipEmptyParts);
	FileMode mode;
	FileOp op;

	if(linkData[1].compare("fileaccept", Qt::CaseInsensitive) == 0) {
		mode = FM_Receive;
		op = FO_Accept;
	} else if(linkData[1].compare("filedecline", Qt::CaseInsensitive) == 0) {
		mode = FM_Receive;
		op = FO_Decline;
	} else if(linkData[1].compare("filecancel", Qt::CaseInsensitive) == 0) {
		mode = FM_Send;
		op = FO_Cancel;
	} else	// unknown link command
		return;

	//	Remove the link and show a confirmation message.
	updateFileMessage(mode, op, linkData[2]);

	fileOperation(linkData[2], linkData[1]);
}

void lmcMessageLog::log_contentsSizeChanged(QSize size) {
	QWebFrame* frame = page()->mainFrame();
	frame->scroll(0, size.height());
}

void lmcMessageLog::log_linkHovered(const QString& link, const QString& title, const QString& textContent) {
	Q_UNUSED(title);
	Q_UNUSED(textContent);
	linkHovered = !link.isEmpty();
}

void lmcMessageLog::showContextMenu(const QPoint& pos) {
	copyAction->setEnabled(!selectedText().isEmpty());
	copyLinkAction->setEnabled(linkHovered);
	//	Copy Link is currently hidden since it performs the same action as regular Copy
	copyLinkAction->setVisible(false);
	selectAllAction->setEnabled(!page()->mainFrame()->documentElement().findFirst("body").firstChild().isNull());
	contextMenu->exec(mapToGlobal(pos));
}

void lmcMessageLog::copyAction_triggered(void) {
	pageAction(QWebPage::Copy)->trigger();
}

void lmcMessageLog::copyLinkAction_triggered(void) {
	pageAction(QWebPage::CopyLinkToClipboard)->trigger();
}

void lmcMessageLog::selectAllAction_triggered(void) {
	pageAction(QWebPage::SelectAll)->trigger();
}

void lmcMessageLog::appendMessageLog(QString *lpszHtml) {
	QWebFrame* frame = page()->mainFrame();
	QWebElement document = frame->documentElement();
	QWebElement body = document.findFirst("body");
	body.appendInside(*lpszHtml);
}

void lmcMessageLog::removeMessageLog(QString divClass) {
	QWebFrame* frame = page()->mainFrame();
	QWebElement document = frame->documentElement();
	QWebElement body = document.findFirst("body");
	QWebElement element = body.findFirst("div." + divClass);
	element.removeFromDocument();
}

void lmcMessageLog::appendBroadcast(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QDateTime* pTime) {
	Q_UNUSED(lpszUserId);

	decodeMessage(lpszMessage);

	QString html = themeData.pubMsg;
	QString caption = tr("Broadcast message from %1:");
	html.replace("%iconpath%", "qrc"IDR_BROADCASTMSG);
	html.replace("%sender%", caption.arg(*lpszUserName));
	html.replace("%time%", getTimeString(pTime));
	html.replace("%style%", "");
	html.replace("%message%", *lpszMessage);

	appendMessageLog(&html);
}

void lmcMessageLog::appendMessage(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QDateTime* pTime,
								  QFont* pFont, QColor* pColor) {
	QString html = QString::null;
	bool localUser = (lpszUserId->compare(localId) == 0);

	decodeMessage(lpszMessage);

	QString fontStyle = getFontStyle(pFont, pColor, localUser);

	if(lpszUserId->compare(lastId) != 0) {
		html = localUser ? themeData.outMsg : themeData.inMsg;

		//	get the avatar image for this user from the cache folder
		QString filePath = participantAvatars.value(*lpszUserId);
		//	if image not found, use the default avatar image for this user
		QString iconPath = QFile::exists(filePath) ? QUrl::fromLocalFile(filePath).toString() : "qrc"AVT_DEFAULT;

		html.replace("%iconpath%", iconPath);
		html.replace("%sender%", *lpszUserName);
		html.replace("%time%", getTimeString(pTime));
		html.replace("%style%", fontStyle);
		html.replace("%message%", *lpszMessage);

		QWebFrame* frame = page()->mainFrame();
		QWebElement document = frame->documentElement();
		QWebElement body = document.findFirst("body");
		body.appendInside(html);
	} else {
		html = localUser ? themeData.outNextMsg : themeData.inNextMsg;
		html.replace("%time%", getTimeString(pTime));
		html.replace("%style%", fontStyle);
		html.replace("%message%", *lpszMessage);

		QWebFrame* frame = page()->mainFrame();
		QWebElement document = frame->documentElement();
		QWebElement body = document.findFirst("body");
		QWebElement last = body.lastChild();
		QWebElement insert = last.findFirst("div#insert");
		insert.replace(html);
	}

	hasData = true;
}

void lmcMessageLog::appendPublicMessage(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage,
										QDateTime *pTime, QFont *pFont, QColor *pColor) {
	QString html = QString::null;
	bool localUser = (lpszUserId->compare(localId) == 0);

	decodeMessage(lpszMessage);

	QString fontStyle = getFontStyle(pFont, pColor, localUser);

	if(lpszUserId->compare(lastId) != 0) {
		outStyle = !outStyle;
		html = outStyle ? themeData.outMsg : themeData.inMsg;

		//	get the avatar image for this user from the cache folder
		QString filePath = participantAvatars.value(*lpszUserId);
		//	if image not found, use the default avatar image for this user
		QString iconPath = QFile::exists(filePath) ? QUrl::fromLocalFile(filePath).toString() : "qrc"AVT_DEFAULT;

		html.replace("%iconpath%", iconPath);
		html.replace("%sender%", *lpszUserName);
		html.replace("%time%", getTimeString(pTime));
		html.replace("%style%", fontStyle);
		html.replace("%message%", *lpszMessage);

		QWebFrame* frame = page()->mainFrame();
		QWebElement document = frame->documentElement();
		QWebElement body = document.findFirst("body");
		body.appendInside(html);
	} else {
		html = outStyle ? themeData.outNextMsg : themeData.inNextMsg;
		html.replace("%time%", getTimeString(pTime));
		html.replace("%style%", fontStyle);
		html.replace("%message%", *lpszMessage);

		QWebFrame* frame = page()->mainFrame();
		QWebElement document = frame->documentElement();
		QWebElement body = document.findFirst("body");
		QWebElement last = body.lastChild();
		QWebElement insert = last.findFirst("div#insert");
		insert.replace(html);
	}

	hasData = true;
}

// This function is called to display a file request message on chat box
void lmcMessageLog::appendFileMessage(MessageType type, QString* lpszUserName, XmlMessage* pMessage,
									  bool bReload) {
	QString htmlMsg;
	QString caption;
	QString fileId = pMessage->data(XN_FILEID);
	QString tempId;
	QString szStatus;

	htmlMsg = themeData.reqMsg;
	htmlMsg.replace("%iconpath%", "qrc"IDR_FILEMSG);

	FileOp fileOp = (FileOp)Helper::indexOf(FileOpNames, FO_Max, pMessage->data(XN_FILEOP));

	if(type == MT_LocalFile) {
		tempId = "send" + fileId;
		caption = tr("Sending '%1' to %2.");
		htmlMsg.replace("%sender%", caption.arg(pMessage->data(XN_FILENAME), *lpszUserName));
		htmlMsg.replace("%message%", "");
		htmlMsg.replace("%fileid%", tempId);

		switch(fileOp) {
		case FO_Request:
			sendFileMap.insert(fileId, *pMessage);
			pMessage->addData(XN_TEMPID, tempId);

			htmlMsg.replace("%links%", "<a href='lmc://filecancel/" + fileId + "'>" + tr("Cancel") + "</a>");
			break;
		case FO_Cancel:
		case FO_Accept:
		case FO_Decline:
			szStatus = getFileStatusMessage(FM_Send, fileOp);
			htmlMsg.replace("%links%", szStatus);
			break;
		default:
			return;
			break;
		}
	} else if(type == MT_File) {
		tempId = "receive" + fileId;
		if(autoFile) {
			caption = tr("%1 is sending you a file:");
			htmlMsg.replace("%sender%", caption.arg(*lpszUserName));
			htmlMsg.replace("%message%", pMessage->data(XN_FILENAME) + " (" +
				Helper::formatSize(pMessage->data(XN_FILESIZE).toLongLong()) + ")");
			htmlMsg.replace("%fileid%", "");
		} else {
			caption = tr("%1 sends you a file:");
			htmlMsg.replace("%sender%", caption.arg(*lpszUserName));
			htmlMsg.replace("%message%", pMessage->data(XN_FILENAME) + " (" +
				Helper::formatSize(pMessage->data(XN_FILESIZE).toLongLong()) + ")");
			htmlMsg.replace("%fileid%", tempId);
		}

		switch(fileOp) {
		case FO_Request:
			receiveFileMap.insert(fileId, *pMessage);
			pMessage->addData(XN_TEMPID, tempId);

			if(autoFile) {
				htmlMsg.replace("%links%", tr("File has been accepted automatically."));
			} else {
				htmlMsg.replace("%links%", "<a href='lmc://fileaccept/" + fileId + "'>" + tr("Accept") + "</a>&nbsp;&nbsp;" +
					"<a href='lmc://filedecline/" + fileId + "'>" + tr("Decline") + "</a>");
			}

			if(autoFile && !bReload)
				fileOperation(fileId, "fileaccept");
			break;
		case FO_Cancel:
		case FO_Accept:
		case FO_Decline:
			szStatus = getFileStatusMessage(FM_Receive, fileOp);
			htmlMsg.replace("%links%", szStatus);
			break;
		default:
			return;
			break;
		}
	}

	QWebFrame* frame = page()->mainFrame();
	QWebElement document = frame->documentElement();
	QWebElement body = document.findFirst("body");
	body.appendInside(htmlMsg);
}

QString lmcMessageLog::getFontStyle(QFont* pFont, QColor* pColor, bool size) {
	QString style = "font-family:\"" + pFont->family() + "\"; ";
	if(pFont->italic())
		style.append("font-style:italic; ");
	if(pFont->bold())
		style.append("font-weight:bold; ");

	if(size) {
		style.append("font-size:" + QString::number(pFont->pointSize()) + "pt; ");
		style.append("color:" + pColor->name() + "; ");
	}
	else
		style.append(fontStyle[fontSizeVal] + " ");

	if(pFont->strikeOut())
		style.append("text-decoration:line-through; ");
	if(pFont->underline())
		style.append("text-decoration:underline; ");

	return style;
}

QString lmcMessageLog::getFileStatusMessage(FileMode mode, FileOp op) {
	QString message;

	switch(op) {
	case FO_Accept:
		message = (mode == FM_Send) ? tr("Accepted") : tr("You have accepted the file.");
		break;
	case FO_Decline:
		message = (mode == FM_Send) ? tr("Declined") : tr("You have declined the file.");
		break;
	case FO_Cancel:
		message = (mode == FM_Send) ? tr("Canceled") : tr("Canceled");
		break;
	default:
		break;
	}

	return message;
}

QString lmcMessageLog::getChatStateMessage(ChatState chatState) {
	QString message = QString::null;

	switch(chatState) {
	case CS_Composing:
		message = tr("%1 is typing...");
		break;
	case CS_Paused:
		message = tr("%1 has entered text");
		break;
	default:
		break;
	}

	return message;
}

QString lmcMessageLog::getChatRoomMessage(GroupMsgOp op) {
	QString message = QString::null;

	switch(op) {
	case GMO_Join:
		message = tr("%1 has joined this conversation");
		break;
	case GMO_Leave:
		message = tr("%1 has left this conversation");
		break;
	default:
		break;
	}

	return message;
}

void lmcMessageLog::fileOperation(QString fileId, QString action) {
	XmlMessage xmlMessage;

	if(action.compare("fileaccept", Qt::CaseInsensitive) == 0) {
		XmlMessage fileData = receiveFileMap.value(fileId);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Receive]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Accept]);
		xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
		xmlMessage.addData(XN_FILEPATH, fileData.data(XN_FILEPATH));
		xmlMessage.addData(XN_FILENAME, fileData.data(XN_FILENAME));
		xmlMessage.addData(XN_FILESIZE, fileData.data(XN_FILESIZE));
	}
	else if(action.compare("filedecline", Qt::CaseInsensitive) == 0) {
		XmlMessage fileData = receiveFileMap.value(fileId);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Receive]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Decline]);
		xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
	}
	else if(action.compare("filecancel", Qt::CaseInsensitive) == 0) {
		XmlMessage fileData = receiveFileMap.value(fileId);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Send]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Cancel]);
		xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
	}

	emit messageSent(MT_LocalFile, &peerId, &xmlMessage);
}

//	Called when message received, before adding to message log
void lmcMessageLog::decodeMessage(QString* lpszMessage) {
	if(trimMessage)
		*lpszMessage = lpszMessage->trimmed();

	ChatHelper::makeHtmlSafe(lpszMessage);

	//	if smileys are enabled, replace text emoticons with corresponding images
	if(showSmiley)
		ChatHelper::decodeSmileys(lpszMessage);

	lpszMessage->replace("\n", "<br/>");
	if(allowLinks) {
		lpszMessage->replace(QRegExp("(((https|http|ftp|file|smb):[/][/]|www.)[\\w\\d:#@%/;$()~_?\\+-=\\\\\\.&]*)"),
							 "<a href='\\1'>\\1</a>");
		if(pathToLink)
			lpszMessage->replace(QRegExp("((\\\\\\\\[\\w-]+\\\\[^\\\\/:*?<>|""]+)((?:\\\\[^\\\\/:*?<>|""]+)*\\\\?)$)"),
								 "<a href='file:\\1'>\\1</a>");
	}
}

QString lmcMessageLog::getTimeString(QDateTime* pTime) {
	QString szTimeStamp;
	if(messageTime) {
		szTimeStamp.append("(");
		if(messageDate)
			szTimeStamp.append(pTime->date().toString(Qt::SystemLocaleShortDate) + "&nbsp;");
		szTimeStamp.append(pTime->time().toString(Qt::SystemLocaleShortDate) + ")&nbsp;");
	}

	return szTimeStamp;
}

void lmcMessageLog::setUIText(void) {
	copyAction->setText(tr("&Copy"));
	selectAllAction->setText(tr("Select &All"));
	reloadMessageLog();
}
