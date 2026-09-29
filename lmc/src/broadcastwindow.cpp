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


#include <QMessageBox>
#include "broadcastwindow.h"

//	constructor
lmcBroadcastWindow::lmcBroadcastWindow(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);

	//	Destroy the window when it closes
	setAttribute(Qt::WA_DeleteOnClose, true);
	
	//	set up the initial default size of splitter panels
	//	left panel takes up 60% of total width, right panel the rest
	QList<int> sizes;
	sizes.append(width() * 0.6);
	sizes.append(width() - width() * 0.6 - ui.splitter->handleWidth());
	ui.splitter->setSizes(sizes);

	connect(ui.btnSelectAll, SIGNAL(clicked()), this, SLOT(btnSelectAll_clicked()));
	connect(ui.btnSelectNone, SIGNAL(clicked()), this, SLOT(btnSelectNone_clicked()));
	connect(ui.tvUserList, SIGNAL(itemChanged(QTreeWidgetItem*, int)), 
		this, SLOT(tvUserList_itemChanged(QTreeWidgetItem*, int)));
	connect(ui.btnSend, SIGNAL(clicked()), this, SLOT(btnSend_clicked()));

	//	event filters for handling keyboard input
	ui.txtMessage->installEventFilter(this);
	ui.tvUserList->installEventFilter(this);
    ui.btnSend->installEventFilter(this);
    ui.btnCancel->installEventFilter(this);
    ui.btnSelectAll->installEventFilter(this);
    ui.btnSelectNone->installEventFilter(this);

	ui.lblInfo->setVisible(false);
	infoFlag = IT_Ok;

	parentToggling = false;
	childToggling = false;
}

lmcBroadcastWindow::~lmcBroadcastWindow() {
}

void lmcBroadcastWindow::init(bool connected) {
	createToolBar();

	setWindowIcon(QIcon(IDR_APPICON));
    ui.splitter->setStyleSheet("QSplitter::handle { image: url(" IDR_HGRIP "); }");

	ui.tvUserList->setIconSize(QSize(16, 16));
    ui.tvUserList->header()->setSectionsMovable(false);
	ui.tvUserList->header()->setStretchLastSection(false);
    ui.tvUserList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui.tvUserList->setCheckable(true);

	//	load settings
	pSettings = new lmcSettings();
	restoreGeometry(pSettings->value(IDS_WINDOWBROADCAST).toByteArray());
	ui.splitter->restoreState(pSettings->value(IDS_SPLITTERBROADCAST).toByteArray());
	showSmiley = pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool();
	sendKeyMod = pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool();
	fontSizeVal = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
	pFontGroup->actions()[fontSizeVal]->setChecked(true);
	int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
	ui.tvUserList->setView((UserListView)viewType);

	//	show a message if not connected
	bConnected = connected;
	ui.btnSend->setEnabled(bConnected);
	if(!bConnected)
		showStatus(IT_Disconnected, true);

	setUIText();

	ui.txtMessage->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
	ui.txtMessage->setFocus();
}

void lmcBroadcastWindow::stop(void) {
	//	save window geometry and splitter panel sizes
	pSettings->setValue(IDS_WINDOWBROADCAST, saveGeometry());
	pSettings->setValue(IDS_SPLITTERBROADCAST, ui.splitter->saveState());
}

void lmcBroadcastWindow::show(QList<QTreeWidgetItem*>* pGroupList) {
	QWidget::show();
	
	if(!pGroupList)
		return;

	//	populate the user tree
	ui.tvUserList->clear();
	for(int index = 0; index < pGroupList->count(); index++) {
		QTreeWidgetItem* pItem = pGroupList->value(index);
		pItem->setCheckState(0, Qt::Unchecked);
		for(int childIndex = 0; childIndex < pItem->childCount(); childIndex++)
			pItem->child(childIndex)->setCheckState(0, Qt::Unchecked);
		ui.tvUserList->addTopLevelItem(pItem);
	}
	ui.tvUserList->expandAll();

	btnSelectAll_clicked();
}

void lmcBroadcastWindow::connectionStateChanged(bool connected) {
	bConnected = connected;
	ui.btnSend->setEnabled(bConnected);
	bConnected ? showStatus(IT_Disconnected, false) : showStatus(IT_Disconnected, true);
}

void lmcBroadcastWindow::settingsChanged(void) {
	showSmiley = pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool();
	sendKeyMod = pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool();
	int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
	ui.tvUserList->setView((UserListView)viewType);
}

//	this method receives keyboard events and check if Enter key or Escape key were pressed
//	if so, corresponding functions are called
bool lmcBroadcastWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
	if(pEvent->type() == QEvent::KeyPress) {
		QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
		if(pKeyEvent->key() == Qt::Key_Escape) {
			close();
			return true;
		} else if((pKeyEvent->key() == Qt::Key_Return || pKeyEvent->key() == Qt::Key_Enter) && pObject == ui.txtMessage) {
			bool keyMod = ((pKeyEvent->modifiers() & Qt::ControlModifier) == Qt::ControlModifier);
			if(keyMod == sendKeyMod) {
				sendMessage();
				return true;
			}
			// The TextEdit widget does not insert new line when Ctrl+Enter is pressed
			// So we insert a new line manually
			if(keyMod)
				ui.txtMessage->insertPlainText("\n");
		}
	}

	return false;
}

void lmcBroadcastWindow::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QWidget::changeEvent(pEvent);
}

void lmcBroadcastWindow::closeEvent(QCloseEvent* pEvent) {
	ui.txtMessage->clear();
	btnSelectNone_clicked();

	QWidget::closeEvent(pEvent);
}

//	change the font size of the text box with toolbar button
void lmcBroadcastWindow::btnFontSize_clicked(void) {
	fontSizeVal = (fontSizeVal == FS_COUNT - 1) ? 0 : fontSizeVal + 1;
	pFontGroup->actions()[fontSizeVal]->setChecked(true);
	pbtnFontSize->setText(lmcStrings::fontSize()[fontSizeVal]);
	ui.txtMessage->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
}

//	change the font size of text box through menu
void lmcBroadcastWindow::fontAction_triggered(QAction* action) {
	fontSizeVal = action->data().toInt();
	pbtnFontSize->setText(lmcStrings::fontSize()[fontSizeVal]);
	ui.txtMessage->setStyleSheet("QTextEdit { " + fontStyle[fontSizeVal] + " }");
}

//	insert a smiley into the text box
void lmcBroadcastWindow::smileyAction_triggered(void) {
	//	if smileys are enabled, insert an image else insert a text smiley
	//	nSmiley contains index of smiley
	if(showSmiley) {
		QString htmlPic("<html><head></head><body><img src='" + smileyPic[nSmiley] + "' /></body></html>");
		ui.txtMessage->insertHtml(htmlPic);
	}
	else
		ui.txtMessage->insertPlainText(smileyCode[nSmiley]);
}

//	select all users in the user tree
void lmcBroadcastWindow::btnSelectAll_clicked(void) {
	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++)
		ui.tvUserList->topLevelItem(index)->setCheckState(0, Qt::Checked);
}

//	deselect all users in the user tree
void lmcBroadcastWindow::btnSelectNone_clicked(void) {
	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++)
		ui.tvUserList->topLevelItem(index)->setCheckState(0, Qt::Unchecked);
}

//	event called when the user checks/unchecks a tree item
void lmcBroadcastWindow::tvUserList_itemChanged(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);

	//	if parent tree item was toggled, update all its children to the same state
	//	if a child tree item was toggled, two cases arise:
	//		if all its siblings and it are checked, update its parent to checked
	//		if all its siblings and it are not checked, update its parent to unchecked
	if(item->data(0, TypeRole).toString().compare("Group") == 0 && !childToggling) {
		parentToggling = true;
		for(int index = 0; index < item->childCount(); index++)
			item->child(index)->setCheckState(0, item->checkState(0));
		parentToggling = false;
	} else if(item->data(0, TypeRole).toString().compare("User") == 0 && !parentToggling) {
		childToggling = true;
		int nChecked = 0;
		QTreeWidgetItem* parent = item->parent();
		for(int index = 0; index < parent->childCount(); index++)
			if(parent->child(index)->checkState(0))
				nChecked++;
		Qt::CheckState check = (nChecked == parent->childCount()) ? Qt::Checked : Qt::Unchecked;
		parent->setCheckState(0, check);
		childToggling = false;
	}
}

void lmcBroadcastWindow::btnSend_clicked(void) {
	sendMessage();
}

//	create toolbar and add buttons
void lmcBroadcastWindow::createToolBar(void) {
	//	create the toolbar
	pToolBar = new QToolBar(ui.toolBarWidget);
	pToolBar->setStyleSheet("QToolBar { border: 0px }");
	pToolBar->setIconSize(QSize(16, 16));
	ui.toolBarLayout->addWidget(pToolBar);

	//	create the font menu
	QMenu* pFontMenu = new QMenu(this);
	pFontGroup = new QActionGroup(this);
	connect(pFontGroup, SIGNAL(triggered(QAction*)), this, SLOT(fontAction_triggered(QAction*)));

	for(int index = 0; index < FS_COUNT; index++) {
		QAction* pAction = new QAction(lmcStrings::fontSize()[index], this);
		pAction->setCheckable(true);
		pAction->setData(index);
		pFontGroup->addAction(pAction);
		pFontMenu->addAction(pAction);
	}

	//	create the font tool button
	pbtnFontSize = new QToolButton(pToolBar);
	pbtnFontSize->setToolButtonStyle(Qt::ToolButtonTextOnly);
	pbtnFontSize->setPopupMode(QToolButton::MenuButtonPopup);
	pbtnFontSize->setMenu(pFontMenu);
	connect(pbtnFontSize, SIGNAL(clicked()), this, SLOT(btnFontSize_clicked()));
	pToolBar->addWidget(pbtnFontSize);

	//	create the smiley menu
	lmcImagePickerAction* pSmileyAction = new lmcImagePickerAction(this, smileyPic, SM_COUNT, 19, 10, &nSmiley);
	connect(pSmileyAction, SIGNAL(triggered()), this, SLOT(smileyAction_triggered()));

	QMenu* pSmileyMenu = new QMenu(this);
	pSmileyMenu->addAction(pSmileyAction);

	//	create the smiley tool button
	pbtnSmiley = new lmcToolButton(pToolBar);
	pbtnSmiley->setIcon(QIcon(QPixmap(IDR_SMILEY, "PNG")));
	pbtnSmiley->setPopupMode(QToolButton::InstantPopup);
	pbtnSmiley->setMenu(pSmileyMenu);
	pToolBar->addWidget(pbtnSmiley);
}

void lmcBroadcastWindow::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(tr("Send Broadcast Message"));

	pbtnFontSize->setText(lmcStrings::fontSize()[fontSizeVal]);
	pbtnFontSize->setToolTip(tr("Change Font Size"));
	pbtnSmiley->setToolTip(tr("Insert Smiley"));

	for(int index = 0; index < pFontGroup->actions().count(); index++)
		pFontGroup->actions()[index]->setText(lmcStrings::fontSize()[index]);
}

//	send the broadcast message to all selected users
void lmcBroadcastWindow::sendMessage(void) {
	//	return if text box is empty
	if(ui.txtMessage->document()->isEmpty())
		return;

	//	send only if connected
	if(bConnected) {
		QString szHtmlMessage(ui.txtMessage->toHtml());
		encodeMessage(&szHtmlMessage);
		QTextDocument docMessage;
		docMessage.setHtml(szHtmlMessage);
		QString szMessage(docMessage.toPlainText());
		
		//	send broadcast
		int sendCount = 0;
		XmlMessage xmlMessage;
		xmlMessage.addData(XN_BROADCAST, szMessage);
		for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++) {
			for(int childIndex = 0; childIndex < ui.tvUserList->topLevelItem(index)->childCount(); childIndex++) {
				QTreeWidgetItem* item = ui.tvUserList->topLevelItem(index)->child(childIndex);
				if(item->checkState(0) == Qt::Checked) {
                    QString szUserId = item->data(0, IdRole).toString();
					emit messageSent(MT_Broadcast, &szUserId, &xmlMessage);
					sendCount++;
				}
			}
		}

		if(sendCount == 0) {
			QMessageBox::warning(this, tr("No recipient selected"), 
				tr("Please select at least one recipient to send a broadcast."));
			return;
		}

		ui.txtMessage->clear();
		close();
	}
}

//	Called before sending message
void lmcBroadcastWindow::encodeMessage(QString* lpszMessage) {
	//	replace all emoticon images with corresponding text code
	ChatHelper::encodeSmileys(lpszMessage);
}

//	show a message depending on the connection state
void lmcBroadcastWindow::showStatus(int flag, bool add) {
	infoFlag = add ? infoFlag | flag : infoFlag & ~flag;

	ui.lblInfo->setStyleSheet("QLabel { background-color:white;font-size:9pt; }");
	if(infoFlag & IT_Disconnected) {
		ui.lblInfo->setText("<span style='color:rgb(192, 0, 0);'>" + 
			tr("You are no longer connected. Broadcast message cannot be sent.") + "</span>");
		ui.lblInfo->setVisible(true);
	} else {
		ui.lblInfo->setText(QString());
		ui.lblInfo->setVisible(false);
	}
}
