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


#include <QMenu>
#include <QAction>
#include "messagelog.h"

const QString acceptOp("accept");
const QString declineOp("decline");
const QString cancelOp("cancel");

lmcMessageLog::lmcMessageLog(QWidget *parent) : QWebView(parent) {
	connect(this, SIGNAL(linkClicked(QUrl)), this, SLOT(log_linkClicked(QUrl)));
	connect(this->page()->mainFrame(), SIGNAL(contentsSizeChanged(QSize)),
			this, SLOT(log_contentsSizeChanged(QSize)));
	connect(this->page(), SIGNAL(linkHovered(QString, QString, QString)),
			this, SLOT(log_linkHovered(QString, QString, QString)));

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	setRenderHints(QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    settings()->setObjectCacheCapacities(0, 0, 0);

	createContextMenu();

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
	autoScroll = true;
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

void lmcMessageLog::createContextMenu(void) {
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
            html.replace("%iconpath%", "qrc" IDR_BLANK);
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
        html.replace("%iconpath%", "qrc" IDR_CRITICALMSG);
		html.replace("%sender%", caption.arg(*lpszUserName));
		html.replace("%style%", fontStyle);
		html.replace("%message%", message);
		appendMessageLog(&html);
		lastId  = QString::null;
		break;
	case MT_Error:
		html = themeData.sysMsg;
        html.replace("%iconpath%", "qrc" IDR_CRITICALMSG);
		html.replace("%sender%", tr("Your message was not sent."));
		html.replace("%message%", "");
		appendMessageLog(&html);
		lastId  = QString::null;
		addToLog = false;
		break;
	case MT_File:
    case MT_Folder:
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
            html.replace("%iconpath%", "qrc" IDR_BLANK);
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
			"*{font-size: 9pt;} body {-webkit-nbsp-mode: space; word-wrap: break-word;}"\
			"span.salutation {float:left; font-weight: bold;} span.time {float: right;}"\
			"span.message {clear: both; display: block;} p {border-bottom: 1px solid #CCC;}"\
			"</style></head><body>";

		for(int index = 0; index < messageLog.count(); index++) {
			SingleMessage msg = messageLog.at(index);
			if(msg.type == MT_Message || msg.type == MT_GroupMessage) {
				time.setMSecsSinceEpoch(msg.message.header(XN_TIME).toLongLong());
				QString messageText = msg.message.data(XN_MESSAGE);
				decodeMessage(&messageText, true);
				QString htmlMsg =
					"<p><span class='salutation'>" + msg.userName + ":</span>"\
					"<span class='time'>" + time.time().toString(Qt::SystemLocaleShortDate) + "</span>"\
					"<span class='message'>" + messageText + "</span></p>";
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

void lmcMessageLog::setAutoScroll(bool enable) {
	autoScroll = enable;
}

void lmcMessageLog::abortPendingFileOperations(void) {
    QMap<QString, XmlMessage>::iterator sIndex = sendFileMap.begin();
    while(sIndex != sendFileMap.end()) {
        XmlMessage fileData = sIndex.value();
        FileOp fileOp = (FileOp)Helper::indexOf(FileOpNames, FO_Max, fileData.data(XN_FILEOP));
        if(fileOp == FO_Request) {
            updateFileMessage(FM_Send, FO_Abort, fileData.data(XN_FILEID));
            sIndex.value().removeData(XN_FILEOP);
            sIndex.value().addData(XN_FILEOP, FileOpNames[FO_Abort]);
        }
        sIndex++;
    }
    QMap<QString, XmlMessage>::iterator rIndex = receiveFileMap.begin();
    while(rIndex != receiveFileMap.end()) {
        XmlMessage fileData = rIndex.value();
        FileOp fileOp = (FileOp)Helper::indexOf(FileOpNames, FO_Max, fileData.data(XN_FILEOP));
        if(fileOp == FO_Request) {
            updateFileMessage(FM_Receive, FO_Abort, fileData.data(XN_FILEID));
            rIndex.value().removeData(XN_FILEOP);
            rIndex.value().addData(XN_FILEOP, FileOpNames[FO_Abort]);
        }
        rIndex++;
    }
}

void lmcMessageLog::saveMessageLog(QString filePath) {
    if(messageLog.isEmpty())
        return;

    QDir dir = QFileInfo(filePath).dir();
    if(!dir.exists())
        dir.mkpath(dir.absolutePath());

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return;

    QDataStream stream(&file);
    stream << peerId << peerName << messageLog;

    file.close();
}

void lmcMessageLog::restoreMessageLog(QString filePath, bool reload) {
    messageLog.clear();

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return;

    QDataStream stream(&file);
    stream >> peerId >> peerName >> messageLog;

    file.close();

    if(reload)
        reloadMessageLog();
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

    if(linkData[2].compare(acceptOp) == 0) {
		mode = FM_Receive;
		op = FO_Accept;
    } else if(linkData[2].compare(declineOp) == 0) {
		mode = FM_Receive;
		op = FO_Decline;
    } else if(linkData[2].compare(cancelOp) == 0) {
		mode = FM_Send;
		op = FO_Cancel;
	} else	// unknown link command
		return;

	//	Remove the link and show a confirmation message.
    updateFileMessage(mode, op, linkData[3]);

    fileOperation(linkData[3], linkData[2], linkData[1], mode);
}

void lmcMessageLog::log_contentsSizeChanged(QSize size) {
	if(autoScroll) {
		QWebFrame* frame = page()->mainFrame();
		frame->scroll(0, size.height());
	}
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
	//copyLinkAction->setVisible(false);
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
    html.replace("%iconpath%", "qrc" IDR_BROADCASTMSG);
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
        QString iconPath = QFile::exists(filePath) ? QUrl::fromLocalFile(filePath).toString() : "qrc" AVT_DEFAULT;

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
        QString iconPath = QFile::exists(filePath) ? QUrl::fromLocalFile(filePath).toString() : "qrc" AVT_DEFAULT;

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
    Q_UNUSED(type);
    QString htmlMsg;
	QString caption;
    QString fileId = pMessage->data(XN_FILEID);
	QString tempId;
	QString szStatus;
    QString fileType;

    switch(type) {
    case MT_File:
        fileType = "file";
        break;
    case MT_Folder:
        fileType = "folder";
        break;
    default:
        return;
        break;
    }

	htmlMsg = themeData.reqMsg;
    htmlMsg.replace("%iconpath%", "qrc" IDR_FILEMSG);

	FileOp fileOp = (FileOp)Helper::indexOf(FileOpNames, FO_Max, pMessage->data(XN_FILEOP));
    FileMode fileMode = (FileMode)Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));

    if(fileMode == FM_Send) {
		tempId = "send" + fileId;
		caption = tr("Sending '%1' to %2.");
		htmlMsg.replace("%sender%", caption.arg(pMessage->data(XN_FILENAME), *lpszUserName));
		htmlMsg.replace("%message%", "");
		htmlMsg.replace("%fileid%", tempId);

		switch(fileOp) {
		case FO_Request:
			sendFileMap.insert(fileId, *pMessage);
			pMessage->addData(XN_TEMPID, tempId);
            htmlMsg.replace("%links%", "<a href='lmc://" + fileType + "/" + cancelOp + "/" + fileId + "'>" + tr("Cancel") + "</a>");
			break;
		case FO_Cancel:
		case FO_Accept:
		case FO_Decline:
        case FO_Error:
        case FO_Abort:
        case FO_Complete:
			szStatus = getFileStatusMessage(FM_Send, fileOp);
			htmlMsg.replace("%links%", szStatus);
			break;
		default:
			return;
			break;
		}
    } else {
		tempId = "receive" + fileId;
		if(autoFile) {
            if(type == MT_File)
                caption = tr("%1 is sending you a file:");
            else
                caption = tr("%1 is sending you a folder:");
			htmlMsg.replace("%sender%", caption.arg(*lpszUserName));
			htmlMsg.replace("%message%", pMessage->data(XN_FILENAME) + " (" +
				Helper::formatSize(pMessage->data(XN_FILESIZE).toLongLong()) + ")");
			htmlMsg.replace("%fileid%", "");
		} else {
            if(type == MT_File)
                caption = tr("%1 sends you a file:");
            else
                caption = tr("%1 sends you a folder:");
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
                htmlMsg.replace("%links%", tr("Accepted"));
                if(!bReload)
                    fileOperation(fileId, acceptOp, fileType);
			} else {
                htmlMsg.replace("%links%",
                    "<a href='lmc://" + fileType + "/" + acceptOp + "/" + fileId + "'>" + tr("Accept") + "</a>&nbsp;&nbsp;" +
                    "<a href='lmc://" + fileType + "/" + declineOp + "/" + fileId + "'>" + tr("Decline") + "</a>");
			}
			break;
		case FO_Cancel:
		case FO_Accept:
		case FO_Decline:
        case FO_Error:
        case FO_Abort:
        case FO_Complete:
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
        message = (mode == FM_Send) ? tr("Accepted") : tr("Accepted");
		break;
	case FO_Decline:
        message = (mode == FM_Send) ? tr("Declined") : tr("Declined");
		break;
	case FO_Cancel:
		message = (mode == FM_Send) ? tr("Canceled") : tr("Canceled");
		break;
    case FO_Error:
    case FO_Abort:
        message = (mode == FM_Send) ? tr("Interrupted") : tr("Interrupted");
        break;
    case FO_Complete:
        message = (mode == FM_Send) ? tr("Completed") : tr("Completed");
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

void lmcMessageLog::fileOperation(QString fileId, QString action, QString fileType, FileMode mode) {
    XmlMessage fileData, xmlMessage;

    MessageType type;
    if(fileType.compare("file") == 0)
        type = MT_File;
    else if(fileType.compare("folder") == 0)
        type = MT_Folder;
    else
        return;

    if(action.compare(acceptOp) == 0) {
        fileData = receiveFileMap.value(fileId);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Receive]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Accept]);
        xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
		xmlMessage.addData(XN_FILEPATH, fileData.data(XN_FILEPATH));
		xmlMessage.addData(XN_FILENAME, fileData.data(XN_FILENAME));
		xmlMessage.addData(XN_FILESIZE, fileData.data(XN_FILESIZE));
	}
    else if(action.compare(declineOp) == 0) {
        fileData = receiveFileMap.value(fileId);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Receive]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Decline]);
        xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
	}
    else if(action.compare(cancelOp) == 0) {
        if(mode == FM_Receive)
            fileData = receiveFileMap.value(fileId);
        else
            fileData = sendFileMap.value(fileId);
        xmlMessage.addData(XN_MODE, FileModeNames[mode]);
        xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
        xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Cancel]);
        xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
	}

    emit messageSent(type, &peerId, &xmlMessage);
}

//	Called when message received, before adding to message log
//	The useDefaults parameter is an override flag that will ignore app settings
//	while decoding the message. If the flag is set, message is not trimmed,
//	smileys are left as text and links will be detected and converted.
void lmcMessageLog::decodeMessage(QString* lpszMessage, bool useDefaults) {
	if(!useDefaults && trimMessage)
		*lpszMessage = lpszMessage->trimmed();

	//	The url detection regexps only work with plain text, so link detection is done before
	//	making the text html safe. The converted links are given a "data-isLink" custom
	//	attribute to differentiate them from the message content
	if(useDefaults || allowLinks) {
//		lpszMessage->replace(QRegExp("(((https|http|ftp|file|smb):[/][/]|www.)[\\w\\d:#@%/;$()~_?\\+-=\\\\\\.&]*)"),
//							 "<a href='\\1'>\\1</a>");
		lpszMessage->replace(QRegExp("((?:(?:https?|ftp|file)://|www\\.|ftp\\.)[-A-Z0-9+&@#/%=~_|$?!:,.]*[A-Z0-9+&@#/%=~_|$])", Qt::CaseInsensitive),
							 "<a data-isLink='true' href='\\1'>\\1</a>");
		lpszMessage->replace("<a data-isLink='true' href='www", "<a data-isLink='true' href='http://www");

		if(!useDefaults && pathToLink)
			lpszMessage->replace(QRegExp("((\\\\\\\\[\\w-]+\\\\[^\\\\/:*?<>|""]+)((?:\\\\[^\\\\/:*?<>|""]+)*\\\\?)$)"),
								 "<a data-isLink='true' href='file:\\1'>\\1</a>");
	}

	QString message = QString::null;
	int index = 0;

	while(index < lpszMessage->length()) {
		int aStart = lpszMessage->indexOf("<a data-isLink='true'", index);
		if(aStart != -1) {
			QString messageSegment = lpszMessage->mid(index, aStart - index);
			processMessageText(&messageSegment, useDefaults);
			message.append(messageSegment);
			index = lpszMessage->indexOf("</a>", aStart) + 4;
			QString linkSegment = lpszMessage->mid(aStart, index - aStart);
			message.append(linkSegment);
		} else {
			QString messageSegment = lpszMessage->mid(index);
			processMessageText(&messageSegment, useDefaults);
			message.append(messageSegment);
			break;
		}
	}

	message.replace("\n", "<br/>");

	*lpszMessage = message;
}

void lmcMessageLog::processMessageText(QString* lpszMessageText, bool useDefaults) {
	ChatHelper::makeHtmlSafe(lpszMessageText);
	//	if smileys are enabled, replace text emoticons with corresponding images
	if(!useDefaults && showSmiley)
		ChatHelper::decodeSmileys(lpszMessageText);
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
