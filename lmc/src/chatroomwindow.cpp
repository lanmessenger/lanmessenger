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


#include "chatroomwindow.h"

QString GroupId = "PARTICIPANTS";

lmcChatRoomWindow::lmcChatRoomWindow(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);
	setAcceptDrops(true);

	connect(ui.tvUserList, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
		this, SLOT(tvUserList_itemActivated(QTreeWidgetItem*, int)));
	connect(ui.tvUserList, SIGNAL(itemContextMenu(QTreeWidgetItem*, QPoint&)),
		this, SLOT(tvUserList_itemContextMenu(QTreeWidgetItem*, QPoint&)));

	pMessageLog = new lmcMessageLog(ui.wgtLog);
	ui.logLayout->addWidget(pMessageLog);
	pMessageLog->setAcceptDrops(false);
	connect(pMessageLog, SIGNAL(messageSent(MessageType,QString*,XmlMessage*)),
			this, SLOT(log_sendMessage(MessageType,QString*,XmlMessage*)));

	int bottomPanelHeight = ui.txtMessage->minimumHeight() + ui.lblDividerBottom->minimumHeight() +
			ui.lblDividerTop->minimumHeight() + ui.wgtToolBar->minimumHeight();
	QList<int> sizes;
	sizes.append(height() - bottomPanelHeight - ui.hSplitter->handleWidth());
	sizes.append(bottomPanelHeight);
	ui.hSplitter->setSizes(sizes);
    ui.hSplitter->setStyleSheet("QSplitter::handle { image: url(" IDR_VGRIP "); }");
	sizes.clear();
	sizes.append(width() * 0.7);
	sizes.append(width() - width() * 0.7 - ui.vSplitter->handleWidth());
	ui.vSplitter->setSizes(sizes);
    ui.vSplitter->setStyleSheet("QSplitter::handle { image: url(" IDR_HGRIP "); }");

	ui.lblInfo->setBackgroundRole(QPalette::Base);
	ui.lblInfo->setAutoFillBackground(true);
	ui.lblInfo->setVisible(false);
    pMessageLog->installEventFilter(this);
    ui.tvUserList->installEventFilter(this);
	ui.txtMessage->installEventFilter(this);
	infoFlag = IT_Ok;
	dataSaved = false;
	windowLoaded = false;

	localId = QString();
	localName = QString();
}

lmcChatRoomWindow::~lmcChatRoomWindow() {
}

void lmcChatRoomWindow::init(User* pLocalUser, bool connected, QString thread) {
	localId = pLocalUser->id;
	localName = pLocalUser->name;

	this->pLocalUser = pLocalUser;

	pMessageLog->localId = localId;

    //	get the avatar image for the user
    pMessageLog->participantAvatars.insert(localId, pLocalUser->avatarPath);

	threadId = thread;
	groupMode = !threadId.isNull();

	createUserMenu();
	createSmileyMenu();
	createToolBar();

	bConnected = connected;
	if(!bConnected)
		showStatus(IT_Disconnected, true);

	pSoundPlayer = new lmcSoundPlayer();

	ui.tvUserList->setIconSize(QSize(16, 16));
    ui.tvUserList->header()->setSectionsMovable(false);
	ui.tvUserList->header()->setStretchLastSection(false);
    ui.tvUserList->header()->setSectionResizeMode(0, QHeaderView::Stretch);

	lmcUserTreeWidgetGroupItem *pItem = new lmcUserTreeWidgetGroupItem();
	pItem->setData(0, IdRole, GroupId);
	pItem->setData(0, TypeRole, "Group");
	pItem->setText(0, "Participants");
	pItem->setSizeHint(0, QSize(0, 22));
	ui.tvUserList->addTopLevelItem(pItem);
	ui.tvUserList->expandAll();

	pSettings = new lmcSettings();
	showSmiley = pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool();
	pMessageLog->showSmiley = showSmiley;
	pMessageLog->autoFile = pSettings->value(IDS_AUTOFILE, IDS_AUTOFILE_VAL).toBool();
	pMessageLog->fontSizeVal = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
	pMessageLog->messageTime = pSettings->value(IDS_MESSAGETIME, IDS_MESSAGETIME_VAL).toBool();
	pMessageLog->messageDate = pSettings->value(IDS_MESSAGEDATE, IDS_MESSAGEDATE_VAL).toBool();
	pMessageLog->allowLinks = pSettings->value(IDS_ALLOWLINKS, IDS_ALLOWLINKS_VAL).toBool();
	pMessageLog->pathToLink = pSettings->value(IDS_PATHTOLINK, IDS_PATHTOLINK_VAL).toBool();
	pMessageLog->trimMessage = pSettings->value(IDS_TRIMMESSAGE, IDS_TRIMMESSAGE_VAL).toBool();
	QFont font = QApplication::font();
	font.fromString(pSettings->value(IDS_FONT, IDS_FONT_VAL).toString());
	messageColor = QApplication::palette().text().color();
	messageColor.setNamedColor(pSettings->value(IDS_COLOR, IDS_COLOR_VAL).toString());
	sendKeyMod = pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool();

	if(!groupMode) {
		restoreGeometry(pSettings->value(IDS_WINDOWPUBLICCHAT).toByteArray());
		ui.vSplitter->restoreState(pSettings->value(IDS_SPLITTERPUBLICCHATV).toByteArray());
		ui.hSplitter->restoreState(pSettings->value(IDS_SPLITTERPUBLICCHATH).toByteArray());
	}

	setUIText();

	setMessageFont(font);
	ui.txtMessage->setStyleSheet("QTextEdit {color: " + messageColor.name() + ";}");
	ui.txtMessage->setFocus();

	QString themePath = pSettings->value(IDS_THEME, IDS_THEME_VAL).toString();
	pMessageLog->initMessageLog(themePath);

	int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
	ui.tvUserList->setView((UserListView)viewType);

	//	Destroy the window when it closes if in group mode
	setAttribute(Qt::WA_DeleteOnClose, groupMode);

	//	Add the local user as a participant
	addUser(pLocalUser);
}

void lmcChatRoomWindow::show(void) {
	windowLoaded = true;
	QWidget::show();
}

void lmcChatRoomWindow::stop(void) {
	bool saveHistory = pSettings->value(IDS_HISTORY, IDS_HISTORY_VAL).toBool();
	// Save history if there is conversation to be saved, and option to save history
	// is enabled, and the conversation was a group conversation.
	// dataSaved flag is used to ensure that history is saved only once, when the
	// window is closed by the user or by the application on exit
	if(pMessageLog->hasData && saveHistory && groupMode && !dataSaved) {
		QString szMessageLog = pMessageLog->prepareMessageLogForSave();
		History::save(tr("Group Conversation"), QDateTime::currentDateTime(), &szMessageLog);
		dataSaved = true;
	}

	if(!groupMode && windowLoaded) {
		pSettings->setValue(IDS_WINDOWPUBLICCHAT, saveGeometry());
		pSettings->setValue(IDS_SPLITTERPUBLICCHATV, ui.vSplitter->saveState());
		pSettings->setValue(IDS_SPLITTERPUBLICCHATH, ui.hSplitter->saveState());
	}
}

void lmcChatRoomWindow::addUser(User* pUser) {
	if(!pUser)
		return;

	// Do not add user if user's version is 1.2.10 or less. These versions do not
	// support Public Chat feature.
	if(Helper::compareVersions(pUser->version, "1.2.10") <= 0)
		return;

	//	Do not add user if user is already in the list of participants
	if(peerIds.contains(pUser->id))
		return;

	peerIds.insert(pUser->id, pUser->id);
	peerNames.insert(pUser->id, pUser->name);

	int index = Helper::statusIndexFromCode(pUser->status);

	lmcUserTreeWidgetUserItem *pItem = new lmcUserTreeWidgetUserItem();
	pItem->setData(0, IdRole, pUser->id);
	pItem->setData(0, TypeRole, "User");
	pItem->setData(0, StatusRole, index);
	pItem->setData(0, SubtextRole, pUser->note);
	pItem->setText(0, pUser->name);

	if(index != -1)
		pItem->setIcon(0, QIcon(QPixmap(statusPic[index], "PNG")));

	lmcUserTreeWidgetGroupItem* pGroupItem = (lmcUserTreeWidgetGroupItem*)getGroupItem(&GroupId);
	pGroupItem->addChild(pItem);
	pGroupItem->sortChildren(0, Qt::AscendingOrder);

	// this should be called after item has been added to tree
    setUserAvatar(&pUser->id, &pUser->avatarPath);

	if(groupMode) {
		XmlMessage xmlMessage;
		xmlMessage.addData(XN_THREAD, threadId);
		xmlMessage.addData(XN_GROUPMSGOP, GroupMsgOpNames[GMO_Join]);

		appendMessageLog(MT_Join, &pUser->id, &pUser->name, &xmlMessage);
		setWindowTitle(getWindowTitle());
		emit messageSent(MT_GroupMessage, NULL, &xmlMessage);
	}

	//	Local user cannot participate in public chat if status is offline
	if(!groupMode && pUser->id.compare(localId) == 0) {
		bool offline = (statusType[Helper::statusIndexFromCode(pUser->status)] == StatusTypeOffline);
		ui.txtMessage->setEnabled(!offline);
		ui.txtMessage->setFocus();
	}
}

void lmcChatRoomWindow::updateUser(User* pUser) {
	if(!pUser)
		return;

	QTreeWidgetItem* pItem = getUserItem(&pUser->id);
	if(pItem) {
		updateStatusImage(pItem, &pUser->status);
		pItem->setData(0, StatusRole, Helper::statusIndexFromCode(pUser->status));
		pItem->setData(0, SubtextRole, pUser->note);
		pItem->setText(0, pUser->name);
		QTreeWidgetItem* pGroupItem = pItem->parent();
		pGroupItem->sortChildren(0, Qt::AscendingOrder);
	}

	if(groupMode)
		setWindowTitle(getWindowTitle());

	//	Local user cannot participate in public chat if status is offline
	if(!groupMode && pUser->id.compare(localId) == 0) {
		bool offline = (statusType[Helper::statusIndexFromCode(pUser->status)] == StatusTypeOffline);
		ui.txtMessage->setEnabled(!offline);
		ui.txtMessage->setFocus();
	}
}

void lmcChatRoomWindow::removeUser(QString* lpszUserId) {
	QTreeWidgetItem* pItem = getUserItem(lpszUserId);
	if(!pItem)
		return;

	QTreeWidgetItem* pGroup = pItem->parent();
	pGroup->removeChild(pItem);

	QString userId = peerIds.value(*lpszUserId);
	QString userName = peerNames.value(*lpszUserId);

	peerIds.remove(*lpszUserId);
	peerNames.remove(*lpszUserId);

	if(groupMode) {
		XmlMessage xmlMessage;
		xmlMessage.addData(XN_THREAD, threadId);
		xmlMessage.addData(XN_GROUPMSGOP, GroupMsgOpNames[GMO_Leave]);

		appendMessageLog(MT_Leave, &userId, &userName, &xmlMessage);
		setWindowTitle(getWindowTitle());
	}

	// If the local user is removed for some reason, prevent sending any further messages
	if(userId.compare(localId) == 0)
		ui.txtMessage->setEnabled(false);
}

void lmcChatRoomWindow::receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
	QString title;

	//	if lpszUserId is NULL, the message was sent locally
	QString senderId = lpszUserId ? *lpszUserId : localId;
	QString senderName = lpszUserId ? peerNames.value(senderId) : localName;
	QString data;

	switch(type) {
	case MT_PublicMessage:
		appendMessageLog(type, lpszUserId, &senderName, pMessage);	
		if(isVisible() && !isActiveWindow())
			pSoundPlayer->play(SE_NewPubMessage);
		break;
	case MT_GroupMessage:
		appendMessageLog(type, lpszUserId, &senderName, pMessage);
		if(isHidden() || !isActiveWindow()) {
			pSoundPlayer->play(SE_NewMessage);
			title = tr("%1 says...");
			setWindowTitle(title.arg(senderName));
		}
		break;
    case MT_Avatar:
		data = pMessage->data(XN_FILEPATH);
		// this message may come with or without user id. NULL user id means avatar change
		// by local user, while non NULL user id means avatar change by a peer.
		setUserAvatar(&senderId, &data);
		break;
	case MT_UserName:
		data = pMessage->data(XN_NAME);
		if(peerNames.contains(senderId))
			peerNames.insert(senderId, data);
		pMessageLog->updateUserName(&senderId, &data);
		break;
	case MT_Failed:
		appendMessageLog(type, lpszUserId, &senderName, pMessage);
		break;
	default:
		break;
	}
}

void lmcChatRoomWindow::connectionStateChanged(bool connected) {
	bConnected = connected;
	bConnected ? showStatus(IT_Disconnected, false) : showStatus(IT_Disconnected, true);
}

void lmcChatRoomWindow::settingsChanged(void) {
	showSmiley = pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool();
	pMessageLog->showSmiley = showSmiley;
	pMessageLog->fontSizeVal = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
	pMessageLog->autoFile = pSettings->value(IDS_AUTOFILE, IDS_AUTOFILE_VAL).toBool();
	sendKeyMod = pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool();
	pSoundPlayer->settingsChanged();
	if(localName.compare(pLocalUser->name) != 0) {
		localName = pLocalUser->name;
		updateUser(pLocalUser);
		pMessageLog->updateUserName(&localId, &localName);
	}

	bool msgTime = pSettings->value(IDS_MESSAGETIME, IDS_MESSAGETIME_VAL).toBool();
	bool msgDate = pSettings->value(IDS_MESSAGEDATE, IDS_MESSAGEDATE_VAL).toBool();
	bool allowLinks = pSettings->value(IDS_ALLOWLINKS, IDS_ALLOWLINKS_VAL).toBool();
	bool pathToLink = pSettings->value(IDS_PATHTOLINK, IDS_PATHTOLINK_VAL).toBool();
	bool trim = pSettings->value(IDS_TRIMMESSAGE, IDS_TRIMMESSAGE_VAL).toBool();
	QString theme = pSettings->value(IDS_THEME, IDS_THEME_VAL).toString();
	if(msgTime != pMessageLog->messageTime || msgDate != pMessageLog->messageDate ||
			allowLinks != pMessageLog->allowLinks || pathToLink != pMessageLog->pathToLink ||
			trim != pMessageLog->trimMessage || theme.compare(pMessageLog->themePath) != 0) {
		pMessageLog->messageTime = msgTime;
		pMessageLog->messageDate = msgDate;
		pMessageLog->allowLinks = allowLinks;
		pMessageLog->pathToLink = pathToLink;
		pMessageLog->trimMessage = trim;
		pMessageLog->themePath = theme;
		pMessageLog->reloadMessageLog();
	}

	int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
	ui.tvUserList->setView((UserListView)viewType);
}

void lmcChatRoomWindow::selectContacts(QStringList* selectedContacts) {
	XmlMessage xmlMessage;
	xmlMessage.addData(XN_THREAD, threadId);
	xmlMessage.addData(XN_GROUPMSGOP, GroupMsgOpNames[GMO_Request]);

	for(int index = 0; index < selectedContacts->count(); index++) {
		QString userId = selectedContacts->value(index);
		emit messageSent(MT_GroupMessage, &userId, &xmlMessage);
	}
}

bool lmcChatRoomWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
    if(pEvent->type() == QEvent::KeyPress) {
		QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
        if(pObject == ui.txtMessage) {
            if(pKeyEvent->key() == Qt::Key_Return || pKeyEvent->key() == Qt::Key_Enter) {
                bool keyMod = ((pKeyEvent->modifiers() & Qt::ControlModifier) == Qt::ControlModifier);
                if(keyMod == sendKeyMod) {
                    sendMessage();
                    return true;
                }
                // The TextEdit widget does not insert new line when Ctrl+Enter is pressed
                // So we insert a new line manually
                if(keyMod)
                    ui.txtMessage->insertPlainText("\n");
            } else if(pKeyEvent->key() == Qt::Key_Escape) {
                close();
                return true;
            }
        } else {
            if(pKeyEvent->key() == Qt::Key_Escape) {
                close();
                return true;
            }
        }
	}

	return false;
}

void lmcChatRoomWindow::changeEvent(QEvent* pEvent) {
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

void lmcChatRoomWindow::closeEvent(QCloseEvent* pEvent) {
	if(groupMode) {
		XmlMessage xmlMessage;
		xmlMessage.addData(XN_THREAD, threadId);
		xmlMessage.addData(XN_GROUPMSGOP, GroupMsgOpNames[GMO_Leave]);
		emit messageSent(MT_GroupMessage, NULL, &xmlMessage);

		// call stop procedure to save history
		stop();
		emit closed(&threadId);
	}

	QWidget::closeEvent(pEvent);
}

void lmcChatRoomWindow::userConversationAction_triggered(void) {
	QString userId = ui.tvUserList->currentItem()->data(0, IdRole).toString();
	emit chatStarting(&userId);
}

void lmcChatRoomWindow::userFileAction_triggered(void) {
	QString userId = ui.tvUserList->currentItem()->data(0, IdRole).toString();
	QString dir = pSettings->value(IDS_OPENPATH, IDS_OPENPATH_VAL).toString();
	QString fileName = QFileDialog::getOpenFileName(this, QString(), dir);
	if(!fileName.isEmpty()) {
		pSettings->setValue(IDS_OPENPATH, QFileInfo(fileName).dir().absolutePath());
		XmlMessage xmlMessage;
		xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
		xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Request]);
		xmlMessage.addData(XN_FILEPATH, fileName);
        emit messageSent(MT_File, &userId, &xmlMessage);
	}
}

void lmcChatRoomWindow::userInfoAction_triggered(void) {
	QString userId = ui.tvUserList->currentItem()->data(0, IdRole).toString();
	XmlMessage xmlMessage;
	xmlMessage.addData(XN_QUERYOP, QueryOpNames[QO_Get]);
	emit messageSent(MT_Query, &userId, &xmlMessage);
}

void lmcChatRoomWindow::btnFont_clicked(void) {
	bool ok;
	QFont font = ui.txtMessage->font();
	font.setPointSize(ui.txtMessage->fontPointSize());
	QFont newFont = QFontDialog::getFont(&ok, font, this, tr("Select Font"));
	if(ok)
		setMessageFont(newFont);
}

void lmcChatRoomWindow::btnFontColor_clicked(void) {
	QColor color = QColorDialog::getColor(messageColor, this, tr("Select Color"));
	if(color.isValid()) {
		messageColor = color;
		ui.txtMessage->setStyleSheet("QTextEdit {color: " + messageColor.name() + ";}");
	}
}

void lmcChatRoomWindow::btnSave_clicked(void) {
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
			stream << pMessageLog->prepareMessageLogForSave();
		else
			stream << pMessageLog->prepareMessageLogForSave(TextFormat);
		file.close();
	}
}

void lmcChatRoomWindow::smileyAction_triggered(void) {
	//	nSmiley contains index of smiley
	if(showSmiley) {
		QString htmlPic("<html><head></head><body><img src='" + smileyPic[nSmiley] + "' /></body></html>");
		ui.txtMessage->insertHtml(htmlPic);
	}
	else
		ui.txtMessage->insertPlainText(smileyCode[nSmiley]);
}

void lmcChatRoomWindow::addContactAction_triggered(void) {
	QStringList excludeList;

	QHash<QString, QString>::const_iterator index = peerIds.constBegin();
	while (index != peerIds.constEnd()) {
		QString userId = index.value();
		excludeList.append(userId);
		index++;
	}

    emit contactsAdding(&excludeList);
}

void lmcChatRoomWindow::log_sendMessage(MessageType type, QString *lpszUserId, XmlMessage *pMessage) {
	Q_UNUSED(type);
	Q_UNUSED(lpszUserId);
	Q_UNUSED(pMessage);
//	emit messageSent(type, NULL, pMessage);
}

void lmcChatRoomWindow::tvUserList_itemActivated(QTreeWidgetItem* pItem, int column) {
	Q_UNUSED(column);
	if(pItem->data(0, TypeRole).toString().compare("User") == 0) {
		QString szUserId = pItem->data(0, IdRole).toString();
		if(szUserId.compare(localId) != 0)
			emit chatStarting(&szUserId);
	}
}

void lmcChatRoomWindow::tvUserList_itemContextMenu(QTreeWidgetItem* pItem, QPoint& pos) {
	if(!pItem)
		return;

	if(pItem->data(0, TypeRole).toString().compare("User") == 0) {
		for(int index = 0; index < pUserMenu->actions().count(); index++)
			pUserMenu->actions()[index]->setData(pItem->data(0, IdRole));

		QString szUserId = pItem->data(0, IdRole).toString();
		// do not show context menu for local user
		if(szUserId.compare(localId) != 0)
			pUserMenu->exec(pos);
	}
}

void lmcChatRoomWindow::createUserMenu(void) {
	pUserMenu = new QMenu(this);

	userChatAction = pUserMenu->addAction("&Conversation", this, SLOT(userConversationAction_triggered()));
	userFileAction = pUserMenu->addAction("Send &File", this, SLOT(userFileAction_triggered()));
	pUserMenu->addSeparator();
	userInfoAction = pUserMenu->addAction("Get &Information", this, SLOT(userInfoAction_triggered()));
}

void lmcChatRoomWindow::createSmileyMenu(void) {
	pSmileyAction = new lmcImagePickerAction(this, smileyPic, SM_COUNT, 19, 10, &nSmiley);
	connect(pSmileyAction, SIGNAL(triggered()), this, SLOT(smileyAction_triggered()));

	pSmileyMenu = new QMenu(this);
	pSmileyMenu->addAction(pSmileyAction);
}

void lmcChatRoomWindow::createToolBar(void) {
	QToolBar* pLeftBar = new QToolBar(ui.wgtToolBar);
	pLeftBar->setStyleSheet("QToolBar { border: 0px }");
	pLeftBar->setIconSize(QSize(16, 16));
	ui.toolBarLayout->addWidget(pLeftBar);

	pFontAction = pLeftBar->addAction(QIcon(QPixmap(IDR_FONT, "PNG")), "Change Font...",
									  this, SLOT(btnFont_clicked()));
	pFontColorAction = pLeftBar->addAction(QIcon(QPixmap(IDR_FONTCOLOR, "PNG")), "Change Color...",
										   this, SLOT(btnFontColor_clicked()));

	pLeftBar->addSeparator();

	pbtnSmiley = new lmcToolButton(pLeftBar);
	pbtnSmiley->setIcon(QIcon(QPixmap(IDR_SMILEY, "PNG")));
	pbtnSmiley->setPopupMode(QToolButton::InstantPopup);
	pbtnSmiley->setMenu(pSmileyMenu);
	pLeftBar->addWidget(pbtnSmiley);

	pLeftBar->addSeparator();

	pSaveAction = pLeftBar->addAction(QIcon(QPixmap(IDR_SAVE, "PNG")), "&Save As...", this, SLOT(btnSave_clicked()));
	pSaveAction->setShortcut(QKeySequence::Save);
	pSaveAction->setEnabled(false);

	ui.lblDividerTop->setBackgroundRole(QPalette::Light);
	ui.lblDividerTop->setAutoFillBackground(true);
	ui.lblDividerBottom->setBackgroundRole(QPalette::Dark);
	ui.lblDividerBottom->setAutoFillBackground(true);

	if(groupMode) {
		QToolBar* pContactsBar = new QToolBar(ui.wgtContactsBar);
		pContactsBar->setStyleSheet("QToolBar { border: 0px }");
		pContactsBar->setIconSize(QSize(24, 24));
		ui.contactsBarLayout->addWidget(pContactsBar);

		addContactAction = pContactsBar->addAction(QIcon(QPixmap(IDR_ADDCONTACT, "PNG")), "&Add Contacts...",
												   this, SLOT(addContactAction_triggered()));

		QToolButton* pButton = (QToolButton*)pContactsBar->widgetForAction(addContactAction);
		pButton->setAutoRaise(false);
	}
}

void lmcChatRoomWindow::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(getWindowTitle());

	QTreeWidgetItem* pGroupItem = getGroupItem(&GroupId);
	pGroupItem->setText(0, tr("Participants"));

	userChatAction->setText(tr("&Conversation"));
	userFileAction->setText(tr("Send &File"));
	userInfoAction->setText(tr("Get &Information"));
	pbtnSmiley->setToolTip(tr("Insert Smiley"));
	pSaveAction->setText(tr("&Save As..."));
	pSaveAction->setToolTip(tr("Save this conversation"));
	pFontAction->setText(tr("Change Font..."));
	pFontAction->setToolTip(tr("Change message font"));
	pFontColorAction->setText(tr("Change Color..."));
	pFontColorAction->setToolTip(tr("Change message text color"));

	if(groupMode) {
		addContactAction->setText(tr("&Add Contacts..."));
		addContactAction->setToolTip(tr("Add people to this conversation"));
	}

	showStatus(IT_Ok, true);	//	this will force the info label to retranslate
}

void lmcChatRoomWindow::sendMessage(void) {
	if(ui.txtMessage->document()->isEmpty())
		return;

	if(bConnected) {
		QString szHtmlMessage(ui.txtMessage->toHtml());
		encodeMessage(&szHtmlMessage);
		QTextDocument docMessage;
		docMessage.setHtml(szHtmlMessage);
		QString szMessage = docMessage.toPlainText();

		QFont font = ui.txtMessage->font();
		font.setPointSize(ui.txtMessage->fontPointSize());

		MessageType type = groupMode ? MT_GroupMessage : MT_PublicMessage;
		XmlMessage xmlMessage;
		xmlMessage.addHeader(XN_TIME, QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()));
		xmlMessage.addData(XN_FONT, font.toString());
		xmlMessage.addData(XN_COLOR, messageColor.name());
		xmlMessage.addData(XN_MESSAGE, szMessage);
		if(groupMode) {
			xmlMessage.addData(XN_THREAD, threadId);
			xmlMessage.addData(XN_GROUPMSGOP, GroupMsgOpNames[GMO_Message]);
		}

		appendMessageLog(type, &localId, &localName, &xmlMessage);

		if(groupMode) {
			QHash<QString, QString>::const_iterator index = peerIds.constBegin();
			while (index != peerIds.constEnd()) {
				QString userId = index.value();
				emit messageSent(type, &userId, &xmlMessage);
				index++;
			}
		} else
			emit messageSent(type, NULL, &xmlMessage);
	}
	else
		appendMessageLog(MT_Error, NULL, NULL, NULL);

	ui.txtMessage->clear();
	ui.txtMessage->setFocus();
}

//	Called before sending message
void lmcChatRoomWindow::encodeMessage(QString* lpszMessage) {
	//	replace all emoticon images with corresponding text code
	ChatHelper::encodeSmileys(lpszMessage);
}

void lmcChatRoomWindow::appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage) {
	pMessageLog->appendMessageLog(type, lpszUserId, lpszUserName, pMessage);
	if(!pSaveAction->isEnabled())
		pSaveAction->setEnabled(pMessageLog->hasData);
}

void lmcChatRoomWindow::showStatus(int flag, bool add) {
	infoFlag = add ? infoFlag | flag : infoFlag & ~flag;

// TODO
//	int relScrollPos = pMessageLog->page()->mainFrame()->scrollBarMaximum(Qt::Vertical) -
//			pMessageLog->page()->mainFrame()->scrollBarValue(Qt::Vertical);

	//ui.lblInfo->setStyleSheet("QLabel { background-color:white; }");
	if(infoFlag & IT_Disconnected) {
		ui.lblInfo->setText("<span style='color:rgb(96,96,96);'>" + tr("You are no longer connected.") + "</span>");
		ui.lblInfo->setVisible(true);
	} else {
		ui.lblInfo->setText(QString());
		ui.lblInfo->setVisible(false);
	}

// TODO
//	int scrollPos = pMessageLog->page()->mainFrame()->scrollBarMaximum(Qt::Vertical) - relScrollPos;
//	pMessageLog->page()->mainFrame()->setScrollBarValue(Qt::Vertical, scrollPos);
}

QString lmcChatRoomWindow::getWindowTitle(void) {
	QString title;

	if(groupMode) {
		QHash<QString, QString>::const_iterator index = peerNames.constBegin();
		while (index != peerNames.constEnd()) {
			title.append(index.value() + ", ");
			index++;
		}

		//	remove the final comma and space
		title.remove(title.length() - 2, 2);
		title.append(" - ");
		title.append(tr("Conversation"));
	} else
		title = tr("Public Chat");

	return title;
}

void lmcChatRoomWindow::setMessageFont(QFont& font) {
	ui.txtMessage->setFont(font);
	ui.txtMessage->setFontPointSize(font.pointSize());
}

void lmcChatRoomWindow::updateStatusImage(QTreeWidgetItem* pItem, QString* lpszStatus) {
	int index = Helper::statusIndexFromCode(*lpszStatus);
	if(index != -1)
		pItem->setIcon(0, QIcon(QPixmap(statusPic[index], "PNG")));
}

QTreeWidgetItem* lmcChatRoomWindow::getUserItem(QString* lpszUserId) {
	for(int topIndex = 0; topIndex < ui.tvUserList->topLevelItemCount(); topIndex++) {
		for(int index = 0; index < ui.tvUserList->topLevelItem(topIndex)->childCount(); index++) {
			QTreeWidgetItem* pItem = ui.tvUserList->topLevelItem(topIndex)->child(index);
			if(pItem->data(0, IdRole).toString().compare(*lpszUserId) == 0)
				return pItem;
		}
	}

	return NULL;
}

QTreeWidgetItem* lmcChatRoomWindow::getGroupItem(QString* lpszGroupId) {
	for(int topIndex = 0; topIndex < ui.tvUserList->topLevelItemCount(); topIndex++) {
		QTreeWidgetItem* pItem = ui.tvUserList->topLevelItem(topIndex);
		if(pItem->data(0, IdRole).toString().compare(*lpszGroupId) == 0)
			return pItem;
	}

	return NULL;
}

void lmcChatRoomWindow::setUserAvatar(QString* lpszUserId, QString* lpszFilePath) {
	QTreeWidgetItem* pUserItem = getUserItem(lpszUserId);
    if(!pUserItem || !lpszFilePath)
		return;

    QPixmap avatar(*lpszFilePath);
    avatar = avatar.scaled(QSize(32, 32), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    pUserItem->setData(0, AvatarRole, QIcon(avatar));
    pMessageLog->updateAvatar(lpszUserId, lpszFilePath);
}
