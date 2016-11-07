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
}

lmcChatWindow::~lmcChatWindow(void) {
}

void lmcChatWindow::createFontMenu(void) {
	pFontMenu = new QMenu(this);
	pFontGroup = new QActionGroup(this);
	connect(pFontGroup, SIGNAL(triggered(QAction*)), this, SLOT(fontAction_triggered(QAction*)));

	for(int index = 0; index < FS_COUNT; index++) {
		QAction* pAction = new QAction(lmcStrings::fontSize()[index], this);
		pAction->setCheckable(true);
		pAction->setData(index);
		pFontGroup->addAction(pAction);
		pFontMenu->addAction(pAction);
	}
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

	pbtnFontSize = new QToolButton(pToolBar);
	pbtnFontSize->setToolButtonStyle(Qt::ToolButtonTextOnly);
	pbtnFontSize->setPopupMode(QToolButton::MenuButtonPopup);
	pbtnFontSize->setMenu(pFontMenu);
	connect(pbtnFontSize, SIGNAL(clicked()), this, SLOT(btnFontSize_clicked()));
	pToolBar->addWidget(pbtnFontSize);

	pbtnSmiley = new lmcToolButton(pToolBar);
	pbtnSmiley->setIcon(QIcon(IDR_SMILEY));
	pbtnSmiley->setPopupMode(QToolButton::InstantPopup);
	pbtnSmiley->setMenu(pSmileyMenu);
	pToolBar->addWidget(pbtnSmiley);

	pToolBar->addSeparator();

	pFileAction = pToolBar->addAction(QIcon(IDR_FILE), "Send A &File...", this, SLOT(btnFile_clicked()));
	pFileAction->setShortcut(QKeySequence::Open);
	pSaveAction = pToolBar->addAction(QIcon(IDR_SAVE), "&Save As...", this, SLOT(btnSave_clicked()));
	pSaveAction->setShortcut(QKeySequence::Save);
	pSaveAction->setEnabled(false);

	pToolBar->addSeparator();

	pHistoryAction = pToolBar->addAction(QIcon(IDR_HISTORY), "&History", this, SLOT(btnHistory_clicked()));
	pHistoryAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
	pTransferAction = pToolBar->addAction(QIcon(IDR_TRANSFER), "File &Transfers", this, SLOT(btnTransfers_clicked()));
	pTransferAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_J));
}

void lmcChatWindow::setUIText(void) {
	ui.retranslateUi(this);

	QString title = tr("Chat with %1");
	setWindowTitle(title.arg(remoteUserName));

	pbtnFontSize->setText(lmcStrings::fontSize()[fontSizeVal]);
	pbtnFontSize->setToolTip(tr("Change Font Size"));
	pbtnSmiley->setToolTip(tr("Insert Smiley"));
	pFileAction->setText(tr("Send A &File..."));
	pSaveAction->setText(tr("&Save As..."));
	pHistoryAction->setText(tr("&History"));
	pTransferAction->setText(tr("File &Transfers"));
	QString toolTip = tr("Send a file to %1");
	pFileAction->setToolTip(toolTip.arg(remoteUserName));
	pSaveAction->setToolTip(tr("Save this conversation"));
	pHistoryAction->setToolTip(tr("View History"));
	pTransferAction->setToolTip(tr("View File Transfers"));

	showStatus(IT_Ok, true);	//	this will force the info label to retranslate

	for(int index = 0; index < pFontGroup->actions().count(); index++)
		pFontGroup->actions()[index]->setText(lmcStrings::fontSize()[index]);
}

void lmcChatWindow::fontAction_triggered(QAction* pAction) {
	fontSizeVal = pAction->data().toInt();
	pbtnFontSize->setText(lmcStrings::fontSize()[fontSizeVal]);
	ui.txtMessage->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
	ui.txtMessageLog->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
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

void lmcChatWindow::btnFontSize_clicked(void) {
	fontSizeVal = (fontSizeVal == FS_COUNT - 1) ? 0 : fontSizeVal + 1;
	pFontGroup->actions()[fontSizeVal]->setChecked(true);
	pbtnFontSize->setText(lmcStrings::fontSize()[fontSizeVal]);
	ui.txtMessage->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
	ui.txtMessageLog->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
}

void lmcChatWindow::btnFile_clicked(void) {
	QString dir =pSettings->value(IDS_OPENPATH, IDS_OPENPATH_VAL).toString();
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
	remoteUserId = pRemoteUser->id;
	remoteUserName = pRemoteUser->name;
	remoteUserStatus = pRemoteUser->status;
	remoteUserAvatar = pRemoteUser->avatar;
	this->pLocalUser = pLocalUser;

	createFontMenu();
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
	pFontGroup->actions()[fontSizeVal]->setChecked(true);

	setUIText();

	ui.txtMessage->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
	ui.txtMessageLog->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
	ui.txtMessage->setFocus();
}

void lmcChatWindow::stop(void) {
	bool saveHistory = pSettings->value(IDS_HISTORY, IDS_HISTORY_VAL).toBool();
    if(hasData && saveHistory) {
        QString szMessageLog = prepareMessageLogForSave();
        History::save(remoteUserName, QDateTime::currentDateTime(), &szMessageLog);
    }
}

bool lmcChatWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
	if(pObject == ui.txtMessage && pEvent->type() == QEvent::KeyPress) {
		QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
		if(pKeyEvent->key() == Qt::Key_Return) {
			sendMessage();
			return true;
		}
	}

	return false;
}

void lmcChatWindow::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::ActivationChange:
		if(isActiveWindow()) {
			QString title = tr("Chat with %1");
			setWindowTitle(title.arg(remoteUserName));
		}
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
		updateMessageLog(MT_Message, &localUserId, &localUserName, &szMessageCopy, pLocalUser->avatar);
		emit messageSent(MT_Message, &remoteUserId, &szMessage);
	}
	else
		updateMessageLog(MT_Error, 0, 0, 0, 0);

	ui.txtMessage->clear();
	ui.txtMessage->setFocus();
}

void lmcChatWindow::sendFile(QString* lpszFilePath) {
	QFileInfo fileInfo(*lpszFilePath);

	QString fileData = QString::null;
	fileData.append(QString::number(FO_Request) + DELIMITER + Helper::getUuid() + DELIMITER + 
			fileInfo.filePath() + DELIMITER + fileInfo.fileName() + DELIMITER + QString::number(fileInfo.size()));
	
	showInfoMessage(MT_LocalFileReq, &remoteUserName, &fileData);
	emit messageSent(MT_FileReq, &remoteUserId, &fileData);
}

void lmcChatWindow::receiveMessage(MessageType type, QString* lpszUserId, QString* lpszMessage) {
    Q_UNUSED(lpszUserId);

	int index;
	QString title;

	switch(type) {
	case MT_Message:
		updateMessageLog(type, &remoteUserId, &remoteUserName, lpszMessage, remoteUserAvatar);
		if(isHidden() || !isActiveWindow()) {
			pSoundPlayer->play(SE_NewMessage);
			title = tr("%1 says...");
			setWindowTitle(title.arg(remoteUserName));
		}
		break;
	case MT_Broadcast:
		updateMessageLog(type, &remoteUserId, &remoteUserName, lpszMessage, remoteUserAvatar);
		if(isHidden() || !isActiveWindow()) {
			pSoundPlayer->play(SE_NewMessage);
			title = tr("Broadcast from %1");
			setWindowTitle(title.arg(remoteUserName));
		}
		break;
	case MT_Status:
		index = Helper::statusIndexFromCode(*lpszMessage);
		if(index != -1) {
			setWindowIcon(QIcon(bubblePic[index]));
			statusType[index] == StatusTypeOffline ? showStatus(IT_Offline, true) : showStatus(IT_Offline, false);
			statusType[index] == StatusTypeBusy ? showStatus(IT_Busy, true) : showStatus(IT_Busy, false);
			remoteUserStatus = *lpszMessage;
		}
		break;
	case MT_Avatar:
		remoteUserAvatar = (*lpszMessage).toInt();
		break;
	case MT_UserName:
		remoteUserName = *lpszMessage;
		if(lastUserId.compare(remoteUserId) == 0)
			lastUserId = QString::null;
		break;
	case MT_Failed:
		updateMessageLog(type, &remoteUserId, &remoteUserName, lpszMessage, 0);
		break;
	case MT_FileReq:
		showInfoMessage(type, &remoteUserName, lpszMessage);
		if(isHidden() || !isActiveWindow()) {
			pSoundPlayer->play(SE_NewFile);
			title = tr("%1 sends a file...");
			setWindowTitle(title.arg(remoteUserName));
		}
		break;
	case MT_LocalFileReq:
		sendFile(lpszMessage);
		break;
	case MT_FileOp:
        processFileOp(lpszMessage);
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
	showSmiley = pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool();
	pSoundPlayer->settingsChanged();
	QString userName = pSettings->value(IDS_USERNAME, IDS_USERNAME_VAL).toString();
	if(userName.isEmpty())
		userName = Helper::getLogonName();
	if(localUserName.compare(userName) != 0) {
		localUserName = userName;
		if(lastUserId.compare(localUserId) == 0)
			lastUserId = QString::null;
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

void lmcChatWindow::processFileOp(QString* lpszMessage) {
    QStringList fileData = lpszMessage->split(DELIMITER, QString::SkipEmptyParts);
    int fileOp = fileData[0].toInt();
    QString id = fileData[1];
    QString log = ui.txtMessageLog->toHtml();
    int position = -1;
    switch(fileOp) {
    case FO_CancelSend:
        position = log.lastIndexOf("lmc://accept/" + id);
        updateInfoMessage(position, tr("Canceled"));
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

void lmcChatWindow::updateMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, int nAvatar) {
	QString html;
	QString caption;

	ui.txtMessageLog->moveCursor(QTextCursor::End);

	switch(type) {
	case MT_Message:
		updateMessageLog(lpszUserId, lpszUserName, lpszMessage, nAvatar);
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

void lmcChatWindow::updateMessageLog(QString* lpszUserId, QString* lpszUserName, QString* lpszMessage, int nAvatar) {
	QString html = QString::null;
	QString htmlUser = QString::null;

	if(lpszUserId->compare(lastUserId) != 0) {
		html.append(htmlHeader());

		QString avtSrc;
		avtSrc = (nAvatar < 0) ? AVT_DEFAULT : avtPic[nAvatar];
		htmlUser.append("<tr class='usr'>");
		/*htmlUser.append("<td width='26' rowspan='2'><img width='24' height='24' src='" + avtSrc + "' /></td>");
		htmlUser.append("<td valign='bottom'><span class='usr'>" + *lpszUserName + "</span></td>");
		htmlUser.append("</tr><tr class='usr'>");
		htmlUser.append("<td valign='top'><img width='2048' height='1' src='"IDR_PIXEL"' /></td>");*/
		htmlUser.append("<td width='26'><img width='24' height='24' src='" + avtSrc + "' /></td>");
		htmlUser.append("<td valign='middle'><span class='usr'>" + *lpszUserName + "</span><hr /></td>");
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
	htmlMsg.append("<tr>");
	htmlMsg.append("<td width='8'></td>");
	htmlMsg.append("<td><span class='inf'>" + szTimeStamp + "</span><span class='msg'>" + szMessage + "</span></td>");
	htmlMsg.append("</tr>");

	html.append(htmlUser + htmlMsg + htmlFooter);
	ui.txtMessageLog->insertHtml(html);
	ui.txtMessageLog->append("");

	if(!hasData) {
		hasData = true;
		pSaveAction->setEnabled(true);
	}
}

void lmcChatWindow::showInfoMessage(MessageType type, QString* lpszUserName, QString* lpszMessage) {
	ui.txtMessageLog->moveCursor(QTextCursor::End);

	QString htmlMsg;
	QString caption;
	if(type == MT_LocalFileReq) {
		QStringList fileData = lpszMessage->split(DELIMITER, QString::SkipEmptyParts);
		sendFileMap.insert(fileData[FD_Id], *lpszMessage);

		htmlMsg.append("<img src='"IDR_FILEMSG"' />");
		caption = tr("Sending '%1' to %2.");
		htmlMsg.append("<span id='inf' class = 'inf'>&nbsp;" + caption.arg(fileData[FD_Name], *lpszUserName) + "</span><br/>");
		htmlMsg.append("<a href='lmc://cancel/" + fileData[FD_Id] + "'>" + tr("Cancel") + "</a>");
	} else if(type == MT_FileReq) {
		QStringList fileData = lpszMessage->split(DELIMITER, QString::SkipEmptyParts);
		receiveFileMap.insert(fileData[FD_Id], *lpszMessage);
	
		bool autoFile = pSettings->value(IDS_AUTOFILE, IDS_AUTOFILE_VAL).toBool();
		if(autoFile) {
			htmlMsg.append("<img src='"IDR_FILEMSG"' />");
			caption = tr("%1 is sending you a file:");
			htmlMsg.append("<span id='inf' class='inf'>&nbsp;" + caption.arg(*lpszUserName) + "</span><br/>");
			htmlMsg.append("<span class='fil'>" + fileData[FD_Name] + " (" + 
				Helper::formatSize(fileData[FD_Size].toLongLong()) + ")</span><br/>");
			htmlMsg.append("<span class='inf'>" + tr("File has been accepted automatically.") + "</span>");
		} else {
			htmlMsg.append("<img src='"IDR_FILEMSG"' />");
			caption = tr("%1 sends you a file:");
			htmlMsg.append("<span id='inf' class='inf'>&nbsp;" + caption.arg(*lpszUserName) + "</span><br/>");
			htmlMsg.append("<span class='fil'>" + fileData[FD_Name] + " (" + 
				Helper::formatSize(fileData[FD_Size].toLongLong()) + ")</span><br/>");
			htmlMsg.append("<a href='lmc://accept/" + fileData[FD_Id] + "'>" + tr("Accept") + "</a>&nbsp;&nbsp;" + 
				"<a href='lmc://decline/" + fileData[FD_Id] + "'>" + tr("Decline") + "</a>");
		}

		if(autoFile)
			fileOperation(fileData[FD_Id], "accept");
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

void lmcChatWindow::showStatus(int flag, bool add) {
	infoFlag = add ? infoFlag | flag : infoFlag & ~flag;

	ui.lblInfo->setStyleSheet("QLabel { background-color:white; }");
	if(infoFlag & IT_Disconnected) {
		ui.lblInfo->setText("<span style='color:rgb(96,96,96);'>" + tr("You are no longer connected.") + "</span>");
		ui.lblInfo->setVisible(true);
	} else if(infoFlag & IT_Offline) {
		QString msg = tr("%1 is offline.");
		ui.lblInfo->setText("<span style='color:rgb(96,96,96);'>" + msg.arg(remoteUserName) + "</span>");
		ui.lblInfo->setVisible(true);
	} else if(infoFlag & IT_Busy) {
		QString msg = tr("%1 is busy. You may be interrupting.");
		ui.lblInfo->setText("<span style='color:rgb(192,0,0);'>" + msg.arg(remoteUserName) + "</span>");
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
	QString data = QString::null;

	if(action.compare("accept", Qt::CaseInsensitive) == 0) {
		QStringList fileData = receiveFileMap.value(id).split(DELIMITER, QString::SkipEmptyParts);
		data.append(QString::number(FO_Accept) + DELIMITER + fileData[FD_Id] + DELIMITER + fileData[FD_Path]
		+ DELIMITER + fileData[FD_Name] + DELIMITER + fileData[FD_Size]);
	}
	else if(action.compare("decline", Qt::CaseInsensitive) == 0) {
		QStringList fileData = receiveFileMap.value(id).split(DELIMITER, QString::SkipEmptyParts);
		data.append(QString::number(FO_Decline) + DELIMITER + fileData[FD_Id]);
	}
	else if(action.compare("cancel", Qt::CaseInsensitive) == 0) {
		QStringList fileData = sendFileMap.value(id).split(DELIMITER, QString::SkipEmptyParts);
		data.append(QString::number(FO_CancelSend) + DELIMITER + fileData[FD_Id]);
	}

	emit messageSent(MT_LocalFileOp, &remoteUserId, &data);
}

QString lmcChatWindow::htmlHeader(void) {
	return ui.txtMessageLog->document()->isEmpty() ? htmlHeaderCont : htmlHeaderGap;
}
