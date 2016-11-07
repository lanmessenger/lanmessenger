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


#include <QDesktopServices>
#include <QDesktopWidget>
#include <QUrl>
#include "transferwindow.h"

lmcTransferWindow::lmcTransferWindow(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);
	QRect scr = QApplication::desktop()->screenGeometry();
	move(scr.center() - rect().center());

	connect(ui.lvTransferList, SIGNAL(currentRowChanged(int)), 
		this, SLOT(lvTransferList_currentRowChanged(int)));
	connect(ui.lvTransferList, SIGNAL(activated(const QModelIndex&)),
		this, SLOT(lvTransferList_activated(const QModelIndex&)));
	connect(ui.btnClear, SIGNAL(clicked()), this, SLOT(btnClear_clicked()));

	pendingSendList.clear();
}

lmcTransferWindow::~lmcTransferWindow(void) {
}

void lmcTransferWindow::init(void) {
	setWindowIcon(QIcon(IDR_APPICON));

	createToolBar();
	setButtonState(FileView::TS_Max);

	pSettings = new lmcSettings();
	restoreGeometry(pSettings->value(IDS_WINDOWTRANSFERS).toByteArray());
	setUIText();

	ui.lvTransferList->loadData(StdLocation::transferHistory());
	if(ui.lvTransferList->count() > 0)
		ui.lvTransferList->setCurrentRow(0);

	pSoundPlayer = new lmcSoundPlayer();
}

void lmcTransferWindow::updateList(void) {
	clearList();
}

void lmcTransferWindow::stop(void) {
	//	Cancel all active transfers
	for(int index = 0; index < ui.lvTransferList->count(); index++) {
		FileView* view = ui.lvTransferList->item(index);

		if(view->state < FileView::TS_Complete) {
			int mode = view->mode == FileView::TM_Send ? FM_Send : FM_Receive;
			XmlMessage xmlMessage;
			xmlMessage.addData(XN_MODE, FileModeNames[mode]);
			xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
			xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Cancel]);
			xmlMessage.addData(XN_FILEID, view->id);
			emit messageSent(MT_File, &view->userId, &xmlMessage);

			view->state = FileView::TS_Cancel;
		}
	}

	bool saveHistory = pSettings->value(IDS_FILEHISTORY, IDS_FILEHISTORY_VAL).toBool();
	if(saveHistory)
		ui.lvTransferList->saveData(StdLocation::transferHistory());
	pSettings->setValue(IDS_WINDOWTRANSFERS, saveGeometry());
}

void lmcTransferWindow::createTransfer(FileMode mode, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage) {
	FileView fileView(pMessage->data(XN_FILEID));
	fileView.fileSize = pMessage->data(XN_FILESIZE).toLongLong();
	fileView.sizeDisplay = Helper::formatSize(fileView.fileSize);
	fileView.userId = *lpszUserId;
	fileView.userName = *lpszUserName;
	if(mode == FM_Send) {
		fileView.fileName = pMessage->data(XN_FILENAME);
		fileView.filePath = pMessage->data(XN_FILEPATH);
		fileView.mode = FileView::TM_Send;
		fileView.state = FileView::TS_Wait;
	} else {
		fileView.fileName = getFreeFileName(pMessage->data(XN_FILENAME));
		fileView.filePath =	QDir(StdLocation::fileStorageDir()).absoluteFilePath(fileView.fileName);
		fileView.mode = FileView::TM_Receive;
		fileView.state = FileView::TS_Confirm;
	}
	fileView.fileDisplay = fileView.fileName + " (" + fileView.sizeDisplay + ")";
	fileView.icon = getIcon(fileView.filePath);

	if(mode == FM_Receive) {
		ui.lvTransferList->insertItem(0, &fileView);
		ui.lvTransferList->setCurrentRow(0);
		acceptFile();
	} else {
		pendingSendList.append(fileView);
	}
}

void lmcTransferWindow::receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
    Q_UNUSED(type);
    Q_UNUSED(lpszUserId);
	
	int fileMode = Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));
	int fileOp = Helper::indexOf(FileOpNames, FO_Max, pMessage->data(XN_FILEOP));
	QString id = pMessage->data(XN_FILEID);

	FileView* view = NULL;
    int itemIndex = -1;
	QString trayMsg;

	if(fileOp == FO_Accept) {
		// receiver has accepted
		//	add the transfer to the list view
		for(int index = 0; index < pendingSendList.count(); index++) {
			if(pendingSendList[index].id.compare(id) == 0) {
				FileView view = pendingSendList.takeAt(index);
				view.state = FileView::TS_Send;
				ui.lvTransferList->insertItem(0, &view);
				ui.lvTransferList->setCurrentRow(0);
				break;
			}
		}
		itemIndex = 0;
	} else {
		switch(fileOp) {
		case FO_Decline:
			//	receiver has declined
			view = ui.lvTransferList->item(id, FileView::TM_Send);
			if(!view)
				return;
			itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Send);
			view->state = FileView::TS_Decline;
			break;
		case FO_Cancel:
			if(fileMode == FM_Send) {
				//	sender has canceled
				view = ui.lvTransferList->item(id, FileView::TM_Receive);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Receive);
				view->state = FileView::TS_Cancel;
			} else {
				//	receiver has canceled
				view = ui.lvTransferList->item(id, FileView::TM_Send);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Send);
				view->state = FileView::TS_Cancel;
			}
			break;
		case FO_Progress:
			if(fileMode == FM_Send) {
				view = ui.lvTransferList->item(id, FileView::TM_Send);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Send);
				updateProgress(view, pMessage->data(XN_FILESIZE).toLongLong());
			} else {
				view = ui.lvTransferList->item(id, FileView::TM_Receive);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Receive);
				updateProgress(view, pMessage->data(XN_FILESIZE).toLongLong());
			}
			break;
		case FO_Error:
			if(fileMode == FM_Send) {
				//	sender has aborted due to error
				view = ui.lvTransferList->item(id, FileView::TM_Send);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Send);
				view->state = FileView::TS_Abort;
			} else {
				//	receiver has aborted due to error
				view = ui.lvTransferList->item(id, FileView::TM_Receive);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Receive);
				view->state = FileView::TS_Abort;
			}
			break;
		case FO_Abort:
			if(fileMode == FM_Send) {
				//	sender has aborted due to error
				view = ui.lvTransferList->item(id, FileView::TM_Receive);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Receive);
				view->state = FileView::TS_Abort;
			} else {
				//	receiver has aborted due to error
				view = ui.lvTransferList->item(id, FileView::TM_Send);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Send);
				view->state = FileView::TS_Abort;
			}
			break;
		case FO_Complete:
			if(fileMode == FM_Send) {
				view = ui.lvTransferList->item(id, FileView::TM_Send);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Send);
				view->state = FileView::TS_Complete;
				if(isHidden() || !isActiveWindow()) {
					trayMsg = tr("'%1' has been sent to %2.");
					emit showTrayMessage(TM_Transfer, trayMsg.arg(view->fileName, view->userName), 
						tr("File Transfer Completed"), TMI_Info);
					pSoundPlayer->play(SE_FileDone);
				}
			} else {
				view = ui.lvTransferList->item(id, FileView::TM_Receive);
				if(!view)
					return;
				itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Receive);
				view->filePath = QDir::fromNativeSeparators(pMessage->data(XN_FILEPATH));
				view->icon = getIcon(view->filePath);
				pactShowFolder->setEnabled(QFile::exists(view->filePath));
				view->state = FileView::TS_Complete;
				if(isHidden() || !isActiveWindow()) {
					trayMsg = tr("'%1' has been received from %2.");
					emit showTrayMessage(TM_Transfer, trayMsg.arg(view->fileName, view->userName), 
						tr("File Transfer Completed"), TMI_Info);
					pSoundPlayer->play(SE_FileDone);
				}
			}
			break;
		}
	}

	ui.lvTransferList->itemChanged(itemIndex);

	FileView* current = ui.lvTransferList->currentItem();
	setButtonState(current->state);
}

void lmcTransferWindow::settingsChanged(void) {
	pSoundPlayer->settingsChanged();
}

void lmcTransferWindow::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QWidget::changeEvent(pEvent);
}

void lmcTransferWindow::lvTransferList_currentRowChanged(int currentRow) {
	if(currentRow < 0) {
		setButtonState(FileView::TS_Max);
		return;
	}

	FileView* pFileView = ui.lvTransferList->item(currentRow);
	setButtonState(pFileView->state);
	pactShowFolder->setEnabled(QFile::exists(pFileView->filePath));
}

void lmcTransferWindow::lvTransferList_activated(const QModelIndex& index) {
	FileView* view = ui.lvTransferList->item(index.row());
	
	QDesktopServices::openUrl(QUrl::fromLocalFile(view->filePath));
}

void lmcTransferWindow::btnCancel_clicked(void) {
	FileView* view = ui.lvTransferList->currentItem();

	int mode = view->mode == FileView::TM_Send ? FM_Send : FM_Receive;
	XmlMessage xmlMessage;
	xmlMessage.addData(XN_MODE, FileModeNames[mode]);
	xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
	xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Cancel]);
	xmlMessage.addData(XN_FILEID, view->id);
	emit messageSent(MT_File, &view->userId, &xmlMessage);

	view->state = FileView::TS_Cancel;

	ui.lvTransferList->itemChanged(ui.lvTransferList->currentRow());
	setButtonState(view->state);
}

void lmcTransferWindow::btnRemove_clicked(void) {
	FileView* view = ui.lvTransferList->currentItem();

	if(view->state < FileView::TS_Complete)
		return;

	ui.lvTransferList->removeItem(ui.lvTransferList->currentRow());
}

void lmcTransferWindow::btnClear_clicked(void) {
	clearList();
}

void lmcTransferWindow::btnShowFolder_clicked(void) {
	FileView* view = ui.lvTransferList->currentItem();

	QString path = QFileInfo(view->filePath).dir().path();
	QUrl url;
	// hack to make sure qdesktopservices open a network path
	if(path.startsWith("\\\\") || path.startsWith("//"))
		url.setUrl(QDir::toNativeSeparators(path));
	else
		url = QUrl::fromLocalFile(path);

	QDesktopServices::openUrl(url);
}

void lmcTransferWindow::createToolBar(void) {
	QToolBar* pToolBar = new QToolBar(ui.wgtToolBar);
	pToolBar->setIconSize(QSize(16, 16));
	pToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	ui.toolBarLayout->addWidget(pToolBar);

	pactCancel = pToolBar->addAction(QIcon(QPixmap(IDR_STOP, "PNG")), "Cancel", 
		this, SLOT(btnCancel_clicked()));
	pToolBar->addSeparator();
	pactShowFolder = pToolBar->addAction(QIcon(QPixmap(IDR_FOLDER, "PNG")), "Show In Folder", 
		this, SLOT(btnShowFolder_clicked()));
	pactRemove = pToolBar->addAction(QIcon(QPixmap(IDR_DECLINE, "PNG")), "Remove From List",
		this, SLOT(btnRemove_clicked()));
}

void lmcTransferWindow::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(tr("File Transfers"));

	pactCancel->setText(tr("Cancel"));
	pactShowFolder->setText(tr("Show In Folder"));
	pactRemove->setText(tr("Remove From List"));
}

void lmcTransferWindow::setButtonState(FileView::TransferState state) {
	switch(state) {
	case FileView::TS_Send:
	case FileView::TS_Receive:
		pactCancel->setEnabled(true);
		pactRemove->setEnabled(false);
		break;
	case FileView::TS_Decline:
	case FileView::TS_Complete:
	case FileView::TS_Cancel:
	case FileView::TS_Abort:
		pactCancel->setEnabled(false);
		pactRemove->setEnabled(true);
		break;
	default:
		pactCancel->setEnabled(false);
		pactShowFolder->setEnabled(false);
		pactRemove->setEnabled(false);
		break;
	}
}

QPixmap lmcTransferWindow::getIcon(QString filePath) {
	QFileIconProvider iconProvider;
	QFileInfo fileInfo(filePath);
	QPixmap icon;

	if(QFile::exists(filePath))
		icon = iconProvider.icon(fileInfo).pixmap(32, 32);
	else {
		QString fileName = fileInfo.fileName();
		QString path = QDir::temp().absoluteFilePath(fileName);
		QFile file(path);
		file.open(QIODevice::WriteOnly);
		file.close();
		icon = iconProvider.icon(QFileInfo(path)).pixmap(32, 32);
		QFile::remove(path);
	}

	return icon;
}

QString lmcTransferWindow::formatTime(qint64 size, qint64 speed) {
	int d = 86400;
	int h = 3600;
	int m = 60;

	if(speed == 0)
		return tr("Calculating time");

	int time = size / speed;
	QString s;
	if(time > d) {
		s.append(QString("%1d ").arg(time / d));
		time %= d;
	}
	if(time > h) {
		s.append(QString("%1h ").arg(time / h));
		time %= h;
	}
	if(time > m) {
		s.append(QString("%1m ").arg(time / m));
		time %= m;
	}
	if(time >= 0)
		s.append(QString("%1s ").arg(time));

	return s.trimmed();
}

void lmcTransferWindow::clearList(void) {
	for(int index = 0; index < ui.lvTransferList->count(); index++) {
		FileView* view = ui.lvTransferList->item(index);
		if(view->state < FileView::TS_Complete)
			continue;
		
		ui.lvTransferList->removeItem(index);
		index--;	
	}
}

void lmcTransferWindow::updateProgress(FileView* view, qint64 currentPos) {
	qint64 lastPos = view->position;
	view->position = currentPos;
	view->posDisplay = Helper::formatSize(view->position);
	qint64 lastSpeed = view->speed;
	qint64 timeSpan = view->lastUpdated.msecsTo(QDateTime::currentDateTime());
	view->lastUpdated = QDateTime::currentDateTime();
	qint64 speed = (timeSpan > 0) ? ((view->position - lastPos) / timeSpan) * 1000 : 0;
	view->speed = (speed + lastSpeed) / 2;
	view->speedDisplay = Helper::formatSize(view->speed) + tr("/sec");
	view->timeDisplay = formatTime(view->fileSize - view->position, view->speed);
}

QString lmcTransferWindow::getFreeFileName(QString fileName) {
	QString freeFileName = fileName;

	QString fileDir = StdLocation::fileStorageDir();
	QDir dir(fileDir);
	QString filePath = dir.absoluteFilePath(fileName);
	QString baseName = fileName.mid(0, fileName.lastIndexOf("."));
	QString ext = fileName.mid(fileName.lastIndexOf("."));
	
	int fileCount = 0;
	while(QFile::exists(filePath)) {
		fileCount++;
		freeFileName = baseName + "[" + QString::number(fileCount) + "]" + ext;
		filePath = dir.absoluteFilePath(freeFileName);
	}

	return freeFileName;
}

void lmcTransferWindow::acceptFile(void) {
	FileView* view = ui.lvTransferList->currentItem();
	view->lastUpdated = QDateTime::currentDateTime();

	XmlMessage xmlMessage;
	xmlMessage.addData(XN_MODE, FileModeNames[FM_Receive]);
	xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
	xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Accept]);
	xmlMessage.addData(XN_FILEID, view->id);
	xmlMessage.addData(XN_FILEPATH, view->filePath);
	xmlMessage.addData(XN_FILENAME, view->fileName);
	xmlMessage.addData(XN_FILESIZE, QString::number(view->fileSize));
	emit messageSent(MT_File, &view->userId, &xmlMessage);

	view->state = FileView::TS_Receive;
}
