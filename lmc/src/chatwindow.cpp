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


#include <QDesktopServices>
#include "chatwindow.h"

lmcChatWindow::lmcChatWindow(QWidget *parent, Qt::WFlags flags) : QWidget(parent, flags) {
	ui.setupUi(this);
	setAcceptDrops(true);
	connect(ui.txtMessageLog, SIGNAL(anchorClicked(const QUrl&)), this, SLOT(anchorClicked(const QUrl&)));

	ui.lblInfo->setVisible(false);
	ui.txtMessage->installEventFilter(this);
	hasData = false;
	infoFlag = IT_Ok;
	sendFileMap.clear();
	receiveFileMap.clear();
	lastUserId = QString::null;

	localUserId = QString::null;
	localUserName = QString::null;
	remoteUserIds.clear();
	remoteUserNames.clear();
	remoteUserStatuses.clear();
	remoteUserAvatars.clear();
	threadId = QString::null;
	groupMode = false;
}

lmcChatWindow::~lmcChatWindow(void) {
}

void lmcChatWindow::createSmileyMenu(void) {
	pSmileyAction = new lmcImagePickerAction(this, smileyPic, SM_COUNT, 19, 10, &nSmiley);
	connect(pSmileyAction, SIGNAL(triggered()), this, SLOT(smileyAction_triggered()));

	pSmileyMenu = new QMenu(this);
	pSmileyMenu->addAction(pSmileyAction);
}

void lmcChatWindow::createToolBar(void) {
	pToolBar = new QToolBar(ui.wgtToolBar);
	//pToolBar->setStyleSheet("QToolBar { border: 0px }");
	pToolBar->setIconSize(QSize(16, 16));
	ui.toolBarLayout->addWidget(pToolBar);

	pFontAction = pToolBar->addAction(QIcon(QPixmap(IDR_FONT, "PNG")), "Change Font...", this, SLOT(btnFont_clicked()));
	pFontColorAction = pToolBar->addAction(QIcon(QPixmap(IDR_FONTCOLOR, "PNG")), "Change Color...", this, SLOT(btnFontColor_clicked()));

	pToolBar->addSeparator();

	pbtnSmiley = new lmcToolButton(pToolBar);
	pbtnSmiley->setIcon(QIcon(QPixmap(IDR_SMILEY, "PNG")));
	pbtnSmiley->setPopupMode(QToolButton::InstantPopup);
	pbtnSmiley->setMenu(pSmileyMenu);
	pToolBar->addWidget(pbtnSmiley);

	pToolBar->addSeparator();

	pFileAction = pToolBar->addAction(QIcon(QPixmap(IDR_FILE, "PNG")), "Send A &File...", this, SLOT(btnFile_clicked()));
	pFileAction->setShortcut(QKeySequence::Open);
	pSaveAction = pToolBar->addAction(QIcon(QPixmap(IDR_SAVE, "PNG")), "&Save As...", this, SLOT(btnSave_clicked()));
	pSaveAction->setShortcut(QKeySequence::Save);
	pSaveAction->setEnabled(false);

	pToolBar->addSeparator();

	pHistoryAction = pToolBar->addAction(QIcon(QPixmap(IDR_HISTORY, "PNG")), "&History", this, SLOT(btnHistory_clicked()));
	pHistoryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
	pTransferAction = pToolBar->addAction(QIcon(QPixmap(IDR_TRANSFER, "PNG")), "File &Transfers", this, SLOT(btnTransfers_clicked()));
	pTransferAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_J));
}

void lmcChatWindow::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(getWindowTitle());

	pbtnSmiley->setToolTip(tr("Insert Smiley"));
	pFileAction->setText(tr("Send A &File..."));
	pSaveAction->setText(tr("&Save As..."));
	pHistoryAction->setText(tr("&History"));
	pTransferAction->setText(tr("File &Transfers"));
	if(!groupMode) {
		QString toolTip = tr("Send a file to %1");
		pFileAction->setToolTip(toolTip.arg(remoteUserNames[0]));
	}
	pSaveAction->setToolTip(tr("Save this conversation"));
	pHistoryAction->setToolTip(tr("View History"));
	pTransferAction->setToolTip(tr("View File Transfers"));
	pFontAction->setText(tr("Change Font..."));
	pFontAction->setToolTip(tr("Change message font"));
	pFontColorAction->setText(tr("Change Color..."));
	pFontColorAction->setToolTip(tr("Change message text color"));

	showStatus(IT_Ok, true);	//	this will force the info label to retranslate
}

void lmcChatWindow::smileyAction_triggered(void) {
	//	nSmiley contains index of smiley
	if(showSmiley) {
		QString htmlPic("<html><head></head><body><img src='" + smileyPic[nSmiley] + "' /></body></html>");
		ui.txtMessage->insertHtml(htmlPic);
	}
	else
		ui.txtMessage->insertPlainText(smileyCode[nSmiley]);
}

void lmcChatWindow::anchorClicked(const QUrl& link) {
	QString linkPath = link.toString();
	QStringList linkData = linkPath.split("/", QString::SkipEmptyParts);
	QString htmlMsg = QString::null;

	if(linkData[1].compare("accept", Qt::CaseInsensitive) == 0)
		htmlMsg = "<span class='inf'>" + tr("You have accepted the file.") + "</span>";
	else if(linkData[1].compare("decline", Qt::CaseInsensitive) == 0)
		htmlMsg = "<span class='inf'>" + tr("You have declined the file.") + "</span>";
	else if(linkData[1].compare("cancel", Qt::CaseInsensitive) == 0)
		htmlMsg = "<span class='inf'>" + tr("Canceled") + "</span>";
	else	// unknown link command
		return;

	//	Remove the link and show a confirmation message.
	QTextCursor cursor = ui.txtMessageLog->textCursor();
	cursor.select(QTextCursor::LineUnderCursor);
	cursor.removeSelectedText();
	ui.txtMessageLog->insertHtml(htmlLineHeader + htmlMsg + htmlLineFooter);

	fileOperation(linkData[2], linkData[1]);
}

void lmcChatWindow::btnFont_clicked(void) {
	bool ok;
	QFont font = ui.txtMessage->font();
	font.setPointSize(ui.txtMessage->fontPointSize());
	QFont newFont = QFontDialog::getFont(&ok, font, this, tr("Select Font"));
	if(ok)
		setMessageFont(newFont);
}

void lmcChatWindow::btnFontColor_clicked(void) {
	QColor color = QColorDialog::getColor(ui.txtMessage->textColor(), this, tr("Select Color"));
	if(color.isValid())
		ui.txtMessage->setTextColor(color);
}

void lmcChatWindow::btnFile_clicked(void) {
	QString dir = pSettings->value(IDS_OPENPATH, IDS_OPENPATH_VAL).toString();
	QString fileName = QFileDialog::getOpenFileName(this, QString(), dir);
	if(!fileName.isEmpty()) {
		pSettings->setValue(IDS_OPENPATH, QFileInfo(fileName).dir().absolutePath());
		sendFile(&fileName);
	}
}

void lmcChatWindow::btnSave_clicked(void) {
	QString dir = pSettings->value(IDS_SAVEPATH, IDS_SAVEPATH_VAL).toString();
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Conversation"), dir,
		"HTML File (*.html);;Text File (*.txt)");
	if(!fileName.isEmpty()) {
		pSettings->setValue(IDS_SAVEPATH, QFileInfo(fileName).dir().absolutePath());
		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			return;
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		stream.setGenerateByteOrderMark(true);
		if(fileName.endsWith(".html", Qt::CaseInsensitive))
			stream << prepareMessageLogForSave();
		else
			stream << prepareMessageLogForSave(TextFormat);
		file.close();
	}
}

void lmcChatWindow::btnHistory_clicked(void) {
	emit showHistory();
}

void lmcChatWindow::btnTransfers_clicked(void) {
	emit showTransfers();
}

void lmcChatWindow::init(User* pLocalUser, User* pRemoteUser, bool connected) {
	localUserId = pLocalUser->id;
	localUserName = pLocalUser->name;
	remoteUserIds.append(pRemoteUser->id);
	remoteUserNames.append(pRemoteUser->name);
	remoteUserStatuses.append(pRemoteUser->status);
	remoteUserAvatars.append(pRemoteUser->avatar);
	this->pLocalUser = pLocalUser;

	createSmileyMenu();
	createToolBar();

	bConnected = connected;
	if(!bConnected)
		showStatus(IT_Disconnected, true);
	
	int index = Helper::statusIndexFromCode(pRemoteUser->status);
	if(index != -1) {
		setWindowIcon(QIcon(bubblePic[index]));
		if(statusType[index] == StatusTypeOffline)
			showStatus(IT_Offline, true);
		else if(statusType[index] == StatusTypeBusy)
			showStatus(IT_Busy, true);
	}

	pSoundPlayer = new lmcSoundPlayer();

	pSettings = new lmcSettings();
	showSmiley = pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool();
	fontSizeVal = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
	QFont font = QApplication::font();
	font.fromString(pSettings->value(IDS_FONT, IDS_FONT_VAL).toString());
	QColor color = QApplication::palette().text().color();
	color.setNamedColor(pSettings->value(IDS_COLOR, IDS_COLOR_VAL).toString());

	setUIText();

	setMessageFont(font);
	ui.txtMessage->setTextColor(color);
	ui.txtMessage->setFocus();
}

void lmcChatWindow::stop(void) {
	bool saveHistory = pSettings->value(IDS_HISTORY, IDS_HISTORY_VAL).toBool();
    if(hasData && saveHistory) {
        QString szMessageLog = prepareMessageLogForSave();
		if(!groupMode)
			History::save(remoteUserNames[0], QDateTime::currentDateTime(), &szMessageLog);
		else
			History::save(tr("Group Conversation"), QDateTime::currentDateTime(), &szMessageLog);
    }
}

bool lmcChatWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
	if(pObject == ui.txtMessage && pEvent->type() == QEvent::KeyPress) {
		QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
		if(pKeyEvent->key() == Qt::Key_Return || pKeyEvent->key() == Qt::Key_Enter) {
			sendMessage();
			return true;
		}
	}

	return false;
}

void lmcChatWindow::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::ActivationChange:
		if(isActiveWindow())
			setWindowTitle(getWindowTitle());
		break;
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QWidget::changeEvent(pEvent);
}

void lmcChatWindow::dragEnterEvent(QDragEnterEvent* pEvent) {
	if(pEvent->mimeData()->hasFormat("text/uri-list"))
		pEvent->acceptProposedAction();
}

void lmcChatWindow::dropEvent(QDropEvent* pEvent) {
	QList<QUrl> urls = pEvent->mimeData()->urls();
	if(urls.isEmpty())
		return;

	QString fileName = urls.first().toLocalFile();
	if(fileName.isEmpty())
		return;

	if(!QFile::exists(fileName))
		return;

	sendFile(&fileName);
}

void lmcChatWindow::sendMessage(void) {
	if(ui.txtMessage->document()->isEmpty())
		return;

	if(bConnected) {
		QString szHtmlMessage(ui.txtMessage->toHtml());
		encodeMessage(&szHtmlMessage);
		QTextDocument docMessage;
		docMessage.setHtml(szHtmlMessage);
		QString szMessage(docMessage.toPlainText());
	
		QString szMessageCopy = szMessage;
		QFont font = ui.txtMessage->font();
		font.setPointSize(ui.txtMessage->fontPointSize());
		QColor color = ui.txtMessage->textColor();
		updateMessageLog(MT_Message, &localUserId, &localUserName, &szMessageCopy, &font, &color);
		
		MessageType type = groupMode ? MT_GroupMessage : MT_Message;
		XmlMessage xmlMessage;
		xmlMessage.addData(XN_FONT, font.toString());
		xmlMessage.addData(XN_COLOR, color.name());
		if(groupMode) {
			xmlMessage.addData(XN_THREAD, threadId);
			xmlMessage.addData(XN_GROUPMESSAGE, szMessage);
		} else
			xmlMessage.addData(XN_MESSAGE, szMessage);
		for(int index = 0; index < remoteUserIds.length(); index++)
			emit messageSent(type, &remoteUserIds[index], &xmlMessage);
	}
	else
		updateMessageLog(MT_Error, NULL, NULL, NULL, NULL, NULL);

	ui.txtMessage->clear();
	ui.txtMessage->setFocus();
}

void lmcChatWindow::sendFile(QString* lpszFilePath) {
	QFileInfo fileInfo(*lpszFilePath);

	XmlMessage xmlMessage;
	xmlMessage.addData(XN_MODE, FileModeNames[FM_Send]);
	xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
	xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Request]);
	xmlMessage.addData(XN_FILEID, Helper::getUuid());
	xmlMessage.addData(XN_FILEPATH, fileInfo.filePath());
	xmlMessage.addData(XN_FILENAME, fileInfo.fileName());
	xmlMessage.addData(XN_FILESIZE, QString::number(fileInfo.size()));

	showInfoMessage(MT_LocalFile, &remoteUserNames[0], &xmlMessage);
	emit messageSent(MT_File, &remoteUserIds[0], &xmlMessage);
}

void lmcChatWindow::receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	QString title;
	int statusIndex;

	//	if lpszUserId is NULL, the message was sent locally
	QString remoteUserId = lpszUserId ? *lpszUserId : pLocalUser->id;;
	int index = lpszUserId ? remoteUserIds.indexOf(remoteUserId) : 0;
	QString remoteUserName = lpszUserId ? remoteUserNames[index] : localUserName;
	QString data;
	QFont font;
	QColor color;

	switch(type) {
	case MT_Message:
		data = pMessage->data(XN_MESSAGE);
		font.fromString(pMessage->data(XN_FONT));
		color.setNamedColor(pMessage->data(XN_COLOR));
		updateMessageLog(type, &remoteUserId, &remoteUserName, &data, &font, &color);
		if(isHidden() || !isActiveWindow()) {
			pSoundPlayer->play(SE_NewMessage);
			title = tr("%1 says...");
			setWindowTitle(title.arg(remoteUserName));
		}
		break;
	case MT_Broadcast:
		data = pMessage->data(XN_BROADCAST);
		updateMessageLog(type, &remoteUserId, &remoteUserName, &data, NULL, NULL);
		if(isHidden() || !isActiveWindow()) {
			pSoundPlayer->play(SE_NewMessage);
			title = tr("Broadcast from %1");
			setWindowTitle(title.arg(remoteUserName));
		}
		break;
	case MT_Status:
		data = pMessage->data(XN_STATUS);
		statusIndex = Helper::statusIndexFromCode(data);
		if(statusIndex != -1) {
			setWindowIcon(QIcon(bubblePic[statusIndex]));
			statusType[statusIndex] == StatusTypeOffline ? showStatus(IT_Offline, true) : showStatus(IT_Offline, false);
			statusType[statusIndex] == StatusTypeBusy ? showStatus(IT_Busy, true) : showStatus(IT_Busy, false);
			remoteUserStatuses[index] = data;
		}
		break;
	case MT_LocalAvatar:
		reloadMessageLog();
		break;
	case MT_UserName:
		data = pMessage->data(XN_NAME);
		remoteUserNames[index] = data;
		updateUserName(lpszUserId, &data);
		break;
	case MT_Failed:
		data = pMessage->data(XN_MESSAGE);
		updateMessageLog(type, &remoteUserId, &remoteUserName, &data, NULL, NULL);
		break;
	case MT_File:
		if(pMessage->data(XN_FILEOP) == FileOpNames[FO_Request]) {
			//	a file request has been received
			showInfoMessage(type, &remoteUserName, pMessage);
			if(isHidden() || !isActiveWindow()) {
				pSoundPlayer->play(SE_NewFile);
				title = tr("%1 sends a file...");
				setWindowTitle(title.arg(remoteUserName));
			}
		} else {
			// a file message of op other than request has been received
			processFileOp(pMessage);
		}
		break;
	case MT_LocalFile:
		if(pMessage->data(XN_FILEOP) == FileOpNames[FO_Request]) {
			data = pMessage->data(XN_FILEPATH);
			sendFile(&data);
		}
		break;
    default:
        break;
	}
}

void lmcChatWindow::connectionStateChanged(bool connected) {
	bConnected = connected;
	bConnected ? showStatus(IT_Disconnected, false) : showStatus(IT_Disconnected, true);
}

void lmcChatWindow::settingsChanged(void) {
	fontSizeVal = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
	showSmiley = pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool();
	pSoundPlayer->settingsChanged();
	QString userName = pSettings->value(IDS_USERNAME, IDS_USERNAME_VAL).toString();
	if(userName.isEmpty())
		userName = Helper::getLogonName();
	if(localUserName.compare(userName) != 0) {
		localUserName = userName;
		updateUserName(&localUserId, &localUserName);
	}
}

//	Called before sending message
void lmcChatWindow::encodeMessage(QString* lpszMessage) {
	//	replace all emoticon images with corresponding text code
	ChatHelper::encodeSmileys(lpszMessage);
}

//	Called when message received, before adding to message log
void lmcChatWindow::decodeMessage(QString* lpszMessage) {
	ChatHelper::makeHtmlSafe(lpszMessage);

	//	if smileys are enabled, replace text emoticons with corresponding images
	if(showSmiley)
		ChatHelper::decodeSmileys(lpszMessage);
}

void lmcChatWindow::processFileOp(XmlMessage* pMessage) {
	int fileOp = Helper::indexOf(FileOpNames, FO_Max, pMessage->data(XN_FILEOP));
	int fileMode = Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));
	QString id = pMessage->data(XN_FILEID);
	QString log = ui.txtMessageLog->toHtml();
    int position = -1;

    switch(fileOp) {
    case FO_Cancel:
		if(fileMode == FM_Send) {
			position = log.lastIndexOf("lmc://accept/" + id);
			updateInfoMessage(position, tr("Canceled"));
		}
        break;
    case FO_Accept:
        position = log.lastIndexOf("lmc://cancel/" + id);
        updateInfoMessage(position, tr("Accepted"));
        break;
    case FO_Decline:
        position = log.lastIndexOf("lmc://cancel/" + id);
        updateInfoMessage(position, tr("Declined"));
        break;
    default:
        break;
    }
}

void lmcChatWindow::updateMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, 
	QFont* pFont, QColor* pColor) {
	
	QString html;
	QString caption;

	ui.txtMessageLog->moveCursor(QTextCursor::End);

	switch(type) {
	case MT_Message:
		updateMessageLog(lpszUserId, lpszUserName, lpszMessage, pFont, pColor);
		lastUserId = *lpszUserId;
		break;
	case MT_Broadcast:
		html.append(htmlHeader());
		html.append("<tr class='brd'>");
		html.append("<td width='18'><img src='"IDR_BROADCASTMSG"' /></td>");
		caption = tr("Broadcast message from %1:");
		html.append("<td valign='middle'><span class='inf'>" + caption.arg(*lpszUserName) + "</span></td>");
		html.append("</tr>");
		decodeMessage(lpszMessage);
		html.append("<tr class='brd'>");
		html.append("<td width='18'></td>");
		html.append("<td><span class='msg'>" + *lpszMessage + "</span></td>");
		html.append("</tr>");
		html.append(htmlFooter);
		ui.txtMessageLog->insertHtml(html);
		ui.txtMessageLog->append("");
		lastUserId  = QString::null;
		break;
	case MT_Failed:
		html.append(htmlHeader());
		html.append("<tr class='inf'>");
		html.append("<td width='18'><img src='"IDR_CRITICALMSG"' /></td>");
		caption = tr("This message was not delivered to %1:");
		html.append("<td valign='middle'><span class='inf'>" + caption.arg(*lpszUserName) + "</span></td>");
		html.append("</tr>");
		html.append("<tr class='inf'>");
		html.append("<td width='18'></td>");
		html.append("<td><span class='msg'>" + *lpszMessage + "</span></td>");
		html.append("</tr>");
		html.append(htmlFooter);
		ui.txtMessageLog->insertHtml(html);
		ui.txtMessageLog->append("");
		lastUserId  = QString::null;
		break;
	case MT_Error:
		html.append(htmlHeader());
		html.append("<tr class='inf'>");
		html.append("<td width='18'><img src='"IDR_CRITICALMSG"' /></td>");
		html.append("<td valign='middle'><span class='inf'>" + tr("Your message was not sent.") + "</span></td>");
		html.append("</tr>");
		html.append(htmlFooter);
		ui.txtMessageLog->insertHtml(html);
		ui.txtMessageLog->append("");
		lastUserId  = QString::null;
		break;
    default:
        break;
	}

	ui.txtMessageLog->ensureCursorVisible();
}

void lmcChatWindow::updateMessageLog(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, QFont* pFont, QColor* pColor) {
	QString html = QString::null;
	QString htmlUser = QString::null;

	if(lpszUserId->compare(lastUserId) != 0) {
		html.append(htmlHeader());

		//	get the avatar image for this user from the cache folder
		QDir cacheDir(StdLocation::cacheDir());
		QString fileName = "avt_" + *lpszUserId + ".png";
		QString filePath = cacheDir.absoluteFilePath(fileName);
		//	if image not found, use the default avatar image for this user
		if(!QFile::exists(filePath)) {
			QPixmap avatar(AVT_DEFAULT);
			avatar = avatar.scaled(QSize(AVT_WIDTH, AVT_HEIGHT));
			avatar.save(filePath);
		}
		QString avtSrc = "file:///" + filePath;
		
		htmlUser.append("<tr class='usr'>");
		/*htmlUser.append("<td width='26' rowspan='2'><img width='24' height='24' src='" + avtSrc + "' /></td>");
		htmlUser.append("<td valign='bottom'><span class='usr'>" + *lpszUserName + "</span></td>");
		htmlUser.append("</tr><tr class='usr'>");
		htmlUser.append("<td valign='top'><img width='2048' height='1' src='"IDR_PIXEL"' /></td>");*/
		htmlUser.append("<td width='26'><img width='24' height='24' src='" + avtSrc + "' /></td>");
		htmlUser.append("<td valign='bottom'><p><span id='" + *lpszUserId + "' class='usr'>" + *lpszUserName + "</span></p><hr /></td>");
		htmlUser.append("</tr></table>");
		htmlUser.append("<table width='100%' border='0' cellpadding='0' cellspacing='0'>");
	} else
		html.append(htmlHeaderCont);

	decodeMessage(lpszMessage);
	QString szMessage = *lpszMessage;
	QString szTimeStamp;
	bool messageTime = pSettings->value(IDS_MESSAGETIME, IDS_MESSAGETIME_VAL).toBool();
	bool messageDate = pSettings->value(IDS_MESSAGEDATE, IDS_MESSAGEDATE_VAL).toBool();
	if(messageTime) {
		szTimeStamp.append("(" + QTime::currentTime().toString(Qt::SystemLocaleShortDate));
		if(messageDate)
			szTimeStamp.append("&nbsp;" + QDate::currentDate().toString(Qt::SystemLocaleShortDate));
		szTimeStamp.append(") ");
	}

	QString htmlMsg = QString::null;
	htmlMsg.append("<tr");
	if(lpszUserId->compare(localUserId) != 0)
		htmlMsg.append(" class='cbk'");
	htmlMsg.append(">");
	htmlMsg.append("<td width='8'></td>");
	htmlMsg.append("<td><span class='inf'>" + szTimeStamp + "</span><span class='msg' ");
	bool size = lpszUserId->compare(localUserId) == 0 ? true : false;
	QString fontStyle = getFontStyle(pFont, pColor, size);
	htmlMsg.append(fontStyle);
	htmlMsg.append(">" + szMessage + "</span></td>");
	htmlMsg.append("</tr>");

	QString s = ui.txtMessage->font().toString();

	html.append(htmlUser + htmlMsg + htmlFooter);
	ui.txtMessageLog->insertHtml(html);
	ui.txtMessageLog->append("");

	if(!hasData) {
		hasData = true;
		pSaveAction->setEnabled(true);
	}
}

// This function is called to display a file request message on chat box
void lmcChatWindow::showInfoMessage(MessageType type, QString* lpszUserName, XmlMessage* pMessage) {
	ui.txtMessageLog->moveCursor(QTextCursor::End);

	QString htmlMsg;
	QString caption;
	if(type == MT_LocalFile && pMessage->data(XN_FILEOP) == FileOpNames[FO_Request]) {
		sendFileMap.insert(pMessage->data(XN_FILEID), *pMessage);

		htmlMsg.append("<img src='"IDR_FILEMSG"' />");
		caption = tr("Sending '%1' to %2.");
		htmlMsg.append("<span id='inf' class = 'inf'>&nbsp;" + caption.arg(pMessage->data(XN_FILENAME), *lpszUserName) + "</span><br/>");
		htmlMsg.append("<a href='lmc://cancel/" + pMessage->data(XN_FILEID) + "'>" + tr("Cancel") + "</a>");
	} else if(type == MT_File && pMessage->data(XN_FILEOP) == FileOpNames[FO_Request]) {
		receiveFileMap.insert(pMessage->data(XN_FILEID), *pMessage);
	
		bool autoFile = pSettings->value(IDS_AUTOFILE, IDS_AUTOFILE_VAL).toBool();
		if(autoFile) {
			htmlMsg.append("<img src='"IDR_FILEMSG"' />");
			caption = tr("%1 is sending you a file:");
			htmlMsg.append("<span id='inf' class='inf'>&nbsp;" + caption.arg(*lpszUserName) + "</span><br/>");
			htmlMsg.append("<span class='fil'>" + pMessage->data(XN_FILENAME) + " (" + 
				Helper::formatSize(pMessage->data(XN_FILESIZE).toLongLong()) + ")</span><br/>");
			htmlMsg.append("<span class='inf'>" + tr("File has been accepted automatically.") + "</span>");
		} else {
			htmlMsg.append("<img src='"IDR_FILEMSG"' />");
			caption = tr("%1 sends you a file:");
			htmlMsg.append("<span id='inf' class='inf'>&nbsp;" + caption.arg(*lpszUserName) + "</span><br/>");
			htmlMsg.append("<span class='fil'>" + pMessage->data(XN_FILENAME) + " (" + 
				Helper::formatSize(pMessage->data(XN_FILESIZE).toLongLong()) + ")</span><br/>");
			htmlMsg.append("<a href='lmc://accept/" + pMessage->data(XN_FILEID) + "'>" + tr("Accept") + "</a>&nbsp;&nbsp;" + 
				"<a href='lmc://decline/" + pMessage->data(XN_FILEID) + "'>" + tr("Decline") + "</a>");
		}

		if(autoFile)
			fileOperation(pMessage->data(XN_FILEID), "accept");
	}

	QString html = htmlHeader() + htmlMsg + htmlFooter;
	ui.txtMessageLog->insertHtml(html);
	ui.txtMessageLog->append("");

	lastUserId  = QString::null;

	ui.txtMessageLog->ensureCursorVisible();
}

void lmcChatWindow::updateInfoMessage(int position, const QString szMessage) {
	QString log = ui.txtMessageLog->toHtml();
	if(position < 0)
		return;
	QString newLog = QString::null;
	int bodyPos = log.indexOf("<body", 0) + 5; // 5 is length of '<body'
	newLog.append(log.mid(0, bodyPos));
	newLog.append(" style=\" font-family:'Arial'; font-weight:400; font-style:normal;\"");
	bodyPos = log.indexOf(">", bodyPos);
	position = log.lastIndexOf("<a href", position);
	int lastPos = log.indexOf("</p>", position);
	newLog.append(log.mid(bodyPos, position - bodyPos));
	newLog.append("<span style=\" font-family:'Arial'; color:#606060;\">" + szMessage + "</span>");
	newLog.append(log.mid(lastPos));
	ui.txtMessageLog->setHtml(newLog);
	ui.txtMessageLog->moveCursor(QTextCursor::End);
	ui.txtMessageLog->ensureCursorVisible();
}

void lmcChatWindow::updateUserName(QString* lpszUserId, QString* lpszUserName) {
	QTextCursor cursor = ui.txtMessageLog->textCursor();
	QString log = ui.txtMessageLog->toHtml();

	QString newLog;
	int index = 0;

	QString search = "<a name=\"" + *lpszUserId + "\"></a>";

	while(index < log.length()) {
		int startIndex = index;
		index = log.indexOf(search, index);
		if(index == -1) {
			newLog.append(log.mid(startIndex));
			break;
		}
		//	get position of 'span' tag that comes after the anchor
		index = log.indexOf("<span", index);
		//	get position of closing tag of span
		index = log.indexOf(">", index) + 1; // 1 is length of '>'
		newLog.append(log.mid(startIndex, index - startIndex));
		newLog.append(*lpszUserName + "</span>");
		//	get position of next 'span' tag that comes after the current 'span'
		index = log.indexOf("<span", index);
		//	get position of end tag of span
		index = log.indexOf("</span>", index) + 7; // 7 is length of '</span>'
	}

	ui.txtMessageLog->setHtml(newLog);
	ui.txtMessageLog->setTextCursor(cursor);
	ui.txtMessageLog->ensureCursorVisible();
}

void lmcChatWindow::showStatus(int flag, bool add) {
	infoFlag = add ? infoFlag | flag : infoFlag & ~flag;

	ui.lblInfo->setStyleSheet("QLabel { background-color:white; }");
	if(infoFlag & IT_Disconnected) {
		ui.lblInfo->setText("<span style='color:rgb(96,96,96);'>" + tr("You are no longer connected.") + "</span>");
		ui.lblInfo->setVisible(true);
	} else if(!groupMode && (infoFlag & IT_Offline))  {
		QString msg = tr("%1 is offline.");
		ui.lblInfo->setText("<span style='color:rgb(96,96,96);'>" + msg.arg(remoteUserNames[0]) + "</span>");
		ui.lblInfo->setVisible(true);
	} else if(!groupMode && (infoFlag & IT_Busy)) {
		QString msg = tr("%1 is busy. You may be interrupting.");
		ui.lblInfo->setText("<span style='color:rgb(192,0,0);'>" + msg.arg(remoteUserNames[0]) + "</span>");
		ui.lblInfo->setVisible(true);
	} else {
		ui.lblInfo->setText(QString::null);
		ui.lblInfo->setVisible(false);
	}
}

QString lmcChatWindow::prepareMessageLogForSave(OutputFormat format) {
	QString log = ui.txtMessageLog->toHtml();

	//	replace all emoticon images with corresponding text codes
	encodeMessage(&log);

	QString safeLog;
	safeLog.clear();

	int index = log.indexOf("<body", 0) + 5; // 5 is length of '<body'
	safeLog.append(log.mid(0, index));
	safeLog.append(" style=\" font-family:'Arial'; font-weight:400; font-style:normal;\"");
	index = log.indexOf(">", index);

	//	remove all other images
	while(index < log.length()) {
		int startIndex = index;
		index = log.indexOf("<img", index);
		if(index == -1)	{
			safeLog.append(log.mid(startIndex));
			break;
		}
		safeLog.append(log.mid(startIndex, index - startIndex));
		index = log.indexOf("/>", index) + 2; // 2 is length of '/>'
	}

	log.clear();
	index = 0;
	while(index < safeLog.length()) {
		int startIndex = index;
		index = safeLog.indexOf("<a name=\"inf\"", index);
		if(index == -1)	{
			log.append(safeLog.mid(startIndex));
			break;
		}
		index = safeLog.lastIndexOf("<p", index);
		log.append(safeLog.mid(startIndex, index - startIndex));
		index = safeLog.indexOf("</p>", index) + 4; // 4 is length of '</p>'
	}

	if(format == TextFormat) {
		QTextDocument doc;
		doc.setHtml(log);
		log = doc.toPlainText();
		log = log.remove("\n\n\n");
	}

	return log;
}

void lmcChatWindow::fileOperation(QString id, QString action) {
	XmlMessage xmlMessage;

	if(action.compare("accept", Qt::CaseInsensitive) == 0) {
		XmlMessage fileData = receiveFileMap.value(id);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Receive]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Accept]);
		xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
		xmlMessage.addData(XN_FILEPATH, fileData.data(XN_FILEPATH));
		xmlMessage.addData(XN_FILENAME, fileData.data(XN_FILENAME));
		xmlMessage.addData(XN_FILESIZE, fileData.data(XN_FILESIZE));
	}
	else if(action.compare("decline", Qt::CaseInsensitive) == 0) {
		XmlMessage fileData = receiveFileMap.value(id);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Receive]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Decline]);
		xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
	}
	else if(action.compare("cancel", Qt::CaseInsensitive) == 0) {
		XmlMessage fileData = receiveFileMap.value(id);
		xmlMessage.addData(XN_MODE, FileModeNames[FM_Send]);
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Cancel]);
		xmlMessage.addData(XN_FILEID, fileData.data(XN_FILEID));
	}

	emit messageSent(MT_LocalFile, &remoteUserIds[0], &xmlMessage);
}

QString lmcChatWindow::htmlHeader(void) {
	return ui.txtMessageLog->document()->isEmpty() ? htmlHeaderCont : htmlHeaderGap;
}

QString lmcChatWindow::getWindowTitle(void) {
	QString title = QString::null;
	for(int index = 0; index < remoteUserNames.count(); index++)
		title.append(remoteUserNames[index] + ", ");

	//	remove the final comma and space
	title.remove(title.length() - 2, 2);
	title.append(" - ");
	title.append(tr("Conversation"));
	return title;
}

void lmcChatWindow::reloadMessageLog(void) {
	QTextCursor cursor = ui.txtMessageLog->textCursor();
	QString html = ui.txtMessageLog->toHtml();
	ui.txtMessageLog->setHtml(html);
	ui.txtMessageLog->setTextCursor(cursor);
	ui.txtMessageLog->ensureCursorVisible();
}

QString lmcChatWindow::getFontStyle(QFont* pFont, QColor* pColor, bool size) {
	QString style = "style=\"font-family:'" + pFont->family() + "'; ";
	if(pFont->italic())
		style.append("font-style:italic; ");
	if(pFont->bold())
		style.append("font-weight:bold; ");

	if(size)
		style.append("font-size:" + QString::number(pFont->pointSize()) + "pt; ");
	else
		style.append(fontStyle[fontSizeVal] + " ");

	if(pFont->strikeOut())
		style.append("text-decoration:line-through; ");
	if(pFont->underline())
		style.append("text-decoration:underline; ");

	style.append("color:" + pColor->name() + ";\" ");

	return style;
}

void lmcChatWindow::setMessageFont(QFont& font) {
	ui.txtMessage->setFont(font);
	ui.txtMessage->setFontPointSize(font.pointSize());
}
