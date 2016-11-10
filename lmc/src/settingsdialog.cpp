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


#include <QUrl>
#include <QSound>
#include <QSystemTrayIcon>
#include <QLocale>
#include <QMessageBox>
#include <QAudioDeviceInfo>
#include "settingsdialog.h"

lmcSettingsDialog::lmcSettingsDialog(QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags) {
	ui.setupUi(this);
	//	remove the help button from window button group
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	//	Destroy the window when it closes
	setAttribute(Qt::WA_DeleteOnClose, true);

	pMessageLog = new lmcMessageLog(ui.fraMessageLog);
	ui.logLayout->addWidget(pMessageLog);

	connect(ui.lvCategories, SIGNAL(currentRowChanged(int)), this, SLOT(lvCategories_currentRowChanged(int)));
	connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(btnOk_clicked()));
	connect(ui.chkMessageTime, SIGNAL(toggled(bool)), this, SLOT(chkMessageTime_toggled(bool)));
	connect(ui.chkAllowLinks, SIGNAL(toggled(bool)), this, SLOT(chkAllowLinks_toggled(bool)));
	connect(ui.rdbSysHistoryPath, SIGNAL(toggled(bool)), this, SLOT(rdbSysHistoryPath_toggled(bool)));
	connect(ui.btnHistoryPath, SIGNAL(clicked()), this, SLOT(btnHistoryPath_clicked()));
	connect(ui.btnFilePath, SIGNAL(clicked()), this, SLOT(btnFilePath_clicked()));
	connect(ui.btnClearHistory, SIGNAL(clicked()), this, SLOT(btnClearHistory_clicked()));
	connect(ui.btnClearFileHistory, SIGNAL(clicked()), this, SLOT(btnClearFileHistory_clicked()));
	connect(ui.btnViewFiles, SIGNAL(clicked()), this, SLOT(btnViewFiles_clicked()));
	connect(ui.chkSound, SIGNAL(toggled(bool)), this, SLOT(chkSound_toggled(bool)));
	connect(ui.chkAutoShowFile, SIGNAL(toggled(bool)), this, SLOT(chkAutoShowFile_toggled(bool)));
	connect(ui.btnFont, SIGNAL(clicked()), this, SLOT(btnFont_clicked()));
	connect(ui.btnColor, SIGNAL(clicked()), this, SLOT(btnColor_clicked()));
	connect(ui.btnReset, SIGNAL(clicked()), this, SLOT(btnReset_clicked()));
	connect(ui.cboTheme, SIGNAL(currentIndexChanged(int)), this, SLOT(cboTheme_currentIndexChanged(int)));
	connect(ui.lvBroadcasts, SIGNAL(currentRowChanged(int)), this, SLOT(lvBroadcasts_currentRowChanged(int)));
	connect(ui.txtBroadcast, SIGNAL(textEdited(QString)), this, SLOT(txtBroadcast_textEdited(QString)));
	connect(ui.txtBroadcast, SIGNAL(returnPressed()), this, SLOT(btnAddBroadcast_clicked()));
	connect(ui.btnAddBroadcast, SIGNAL(clicked()), this, SLOT(btnAddBroadcast_clicked()));
	connect(ui.btnDeleteBroadcast, SIGNAL(clicked()), this, SLOT(btnDeleteBroadcast_clicked()));
	connect(ui.lvSounds, SIGNAL(currentRowChanged(int)), this, SLOT(lvSounds_currentRowChanged(int)));
	connect(ui.btnPlaySound, SIGNAL(clicked()), this, SLOT(btnPlaySound_clicked()));
	connect(ui.btnSoundPath, SIGNAL(clicked()), this, SLOT(btnSoundPath_clicked()));
	connect(ui.btnResetSounds, SIGNAL(clicked()), this, SLOT(btnResetSounds_clicked()));
}

lmcSettingsDialog::~lmcSettingsDialog(void) {
}

void lmcSettingsDialog::init(void) {
	QMap<QString, QString> languages;
	//	Loop through available languages and add them to a map. This ensures that
	//	the languages are sorted alphabetically. After that add the sorted items
	//	to the combo box.
	for(int index = 0; index < Application::availableLanguages().count(); index++) {
		QString langCode = Application::availableLanguages().value(index);
		QLocale locale(langCode);
		QString language = QLocale::languageToString(locale.language());
		languages.insert(language, langCode);
	}
	for(int index = 0; index < languages.count(); index++)
		ui.cboLanguage->addItem(languages.keys().value(index), languages.values().value(index));

	for(int index = 0; index < FS_COUNT; index++)
		ui.cboFontSize->addItem(lmcStrings::fontSize()[index], index);

	for(int index = 0; index < SE_Max; index++) {
		QListWidgetItem* pListItem = new QListWidgetItem(ui.lvSounds);
		pListItem->setText(lmcStrings::soundDesc()[index]);
		pListItem->setData(Qt::UserRole, soundFile[index]);
		pListItem->setCheckState(IDS_SOUNDEVENT_VAL);
	}

	Themes themes = lmcTheme::availableThemes();
	for(int index = 0; index < themes.count(); index++)
		ui.cboTheme->addItem(themes.at(index).name, themes.at(index).path);

	for(int index = 0; index < ULV_Max; index++)
		ui.cboUserListView->addItem(lmcStrings::userListView()[index], index);

	fontSize = 0;
	font = QApplication::font();
	color = QApplication::palette().text().color();
	ui.lvCategories->setCurrentRow(0);

	setWindowIcon(QIcon(IDR_APPICON));

	ui.lvCategories->setIconSize(QSize(32, 32));
	ui.lvCategories->item(0)->setIcon(QIcon(QPixmap(IDR_GENERALSET, "PNG")));
	ui.lvCategories->item(1)->setIcon(QIcon(QPixmap(IDR_ACCOUNTSET, "PNG")));
	ui.lvCategories->item(2)->setIcon(QIcon(QPixmap(IDR_MESSAGESET, "PNG")));
	ui.lvCategories->item(3)->setIcon(QIcon(QPixmap(IDR_HISTORYSET, "PNG")));
	ui.lvCategories->item(4)->setIcon(QIcon(QPixmap(IDR_ALERTSET, "PNG")));
	ui.lvCategories->item(5)->setIcon(QIcon(QPixmap(IDR_NETWORKSET, "PNG")));
	ui.lvCategories->item(6)->setIcon(QIcon(QPixmap(IDR_TRANSFERSET, "PNG")));
	ui.lvCategories->item(7)->setIcon(QIcon(QPixmap(IDR_THEMESET, "PNG")));
	ui.lvCategories->item(8)->setIcon(QIcon(QPixmap(IDR_HOTKEYSET, "PNG")));

    setPageHeaderStyle(ui.lblGeneralPage);
    setPageHeaderStyle(ui.lblAccountPage);
    setPageHeaderStyle(ui.lblMessagesPage);
    setPageHeaderStyle(ui.lblHistoryPage);
    setPageHeaderStyle(ui.lblAlertsPage);
    setPageHeaderStyle(ui.lblNetworkPage);
    setPageHeaderStyle(ui.lblFileTransferPage);
    setPageHeaderStyle(ui.lblThemePage);
    setPageHeaderStyle(ui.lblHotkeysPage);

	ui.btnPlaySound->setIcon(QIcon(QPixmap(IDR_PLAY, "PNG")));

	pPortValidator = new QIntValidator(1, 65535, this);
	ui.txtUDPPort->setValidator(pPortValidator);
	ui.txtTCPPort->setValidator(pPortValidator);

	ipRegExp = QRegExp("\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b");
	pIpValidator = new QRegExpValidator(ipRegExp, this);
	ui.txtMulticast->setValidator(pIpValidator);

	pMessageLog->setAutoScroll(false);

	pSettings = new lmcSettings();
	setUIText();
	loadSettings();
}

void lmcSettingsDialog::settingsChanged(void) {
	loadSettings();
}

void lmcSettingsDialog::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QDialog::changeEvent(pEvent);
}

void lmcSettingsDialog::lvCategories_currentRowChanged(int currentRow) {
	ui.stackedWidget->setCurrentIndex(currentRow);
}

void lmcSettingsDialog::btnOk_clicked(void) {
	saveSettings();
}

void lmcSettingsDialog::chkMessageTime_toggled(bool checked) {
    Q_UNUSED(checked);

	ui.chkMessageDate->setEnabled(ui.chkMessageTime->isChecked());
}

void lmcSettingsDialog::chkAllowLinks_toggled(bool checked) {
	Q_UNUSED(checked);

	ui.chkPathToLink->setEnabled(ui.chkAllowLinks->isChecked());
}

void lmcSettingsDialog::rdbSysHistoryPath_toggled(bool checked) {
	ui.txtHistoryPath->setEnabled(!checked);
	ui.btnHistoryPath->setEnabled(!checked);

	if(!checked)
		ui.txtHistoryPath->setText(History::historyFile());
	else
		ui.txtHistoryPath->clear();
}

void lmcSettingsDialog::btnHistoryPath_clicked(void) {
	QString historyPath = QFileDialog::getSaveFileName(this, tr("Save History"),
		ui.txtHistoryPath->text(), "Messenger DB (*.db)");
	if(!historyPath.isEmpty())
		ui.txtHistoryPath->setText(historyPath);
}

void lmcSettingsDialog::btnFilePath_clicked(void) {
	QString filePath = QFileDialog::getExistingDirectory(this, tr("Select folder"),
		ui.txtFilePath->text(), QFileDialog::ShowDirsOnly);
	if(!filePath.isEmpty())
		ui.txtFilePath->setText(filePath);
}

void lmcSettingsDialog::btnClearHistory_clicked(void) {
	QFile::remove(History::historyFile());
	emit historyCleared();
}

void lmcSettingsDialog::btnClearFileHistory_clicked(void) {
	QFile::remove(StdLocation::transferHistory());
	emit fileHistoryCleared();
}

void lmcSettingsDialog::chkSound_toggled(bool checked) {
	ui.lvSounds->setEnabled(checked);
}

void lmcSettingsDialog::chkAutoShowFile_toggled(bool checked) {
	ui.rdbFileTop->setEnabled(checked);
	ui.rdbFileBottom->setEnabled(checked);
}

void lmcSettingsDialog::btnViewFiles_clicked(void) {
	QDesktopServices::openUrl(QUrl::fromLocalFile(ui.txtFilePath->text()));
}

void lmcSettingsDialog::btnFont_clicked(void) {
	bool ok;
	QFont newFont = QFontDialog::getFont(&ok, font, this, tr("Select Font"));
	if(ok)
		font = newFont;
}

void lmcSettingsDialog::btnColor_clicked(void) {
	QColor newColor = QColorDialog::getColor(color, this, tr("Select Color"));
	if(newColor.isValid())
		color = newColor;
}

void lmcSettingsDialog::btnReset_clicked(void) {
	QString message = tr("Are you sure you want to reset your %1 preferences?");
	if(QMessageBox::question(this, tr("Reset Preferences"), message.arg(lmcStrings::appName()), QMessageBox::Yes, QMessageBox::No)
		== QMessageBox::Yes) {
		QFile::remove(pSettings->fileName());
		pSettings->sync();
		accept();
	}
}

void lmcSettingsDialog::cboTheme_currentIndexChanged(int index) {
	QString themePath = ui.cboTheme->itemData(index, Qt::UserRole).toString();

	pMessageLog->fontSizeVal = FS_SMALL;
	pMessageLog->localId = "Myself";
	pMessageLog->peerId = "Jack";
	pMessageLog->messageTime = true;
	pMessageLog->initMessageLog(themePath);

	XmlMessage msg;
	msg.addData(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
	msg.addData(XN_FONT, QFont().toString());
	msg.addData(XN_COLOR, QColor::fromRgb(96, 96, 96).name());

	QString userId = "Jack";
	QString userName = "Jack";

	msg.addData(XN_MESSAGE, "Hello, this is an incoming message.");
	pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);

	msg.removeData(XN_MESSAGE);
	msg.addData(XN_MESSAGE, "Hello, this is a consecutive incoming message.");
	pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);

	msg.removeData(XN_MESSAGE);
	msg.addData(XN_BROADCAST, "This is a broadcast message!");
	pMessageLog->appendMessageLog(MT_Broadcast, &userId, &userName, &msg, true);

	userId = "Myself";
	userName = "Myself";

	msg.removeData(XN_BROADCAST);
	msg.addData(XN_MESSAGE, "Hi, this is an outgoing message.");
	pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);

	msg.removeData(XN_MESSAGE);
	msg.addData(XN_MESSAGE, "Hi, this is a consecutive outgoing message.");
	pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);

	userId = "Jack";
	userName = "Jack";

	msg.removeData(XN_MESSAGE);
	msg.addData(XN_MESSAGE, "This is another incoming message.");
	pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);
}

void lmcSettingsDialog::lvBroadcasts_currentRowChanged(int index) {
	ui.btnDeleteBroadcast->setEnabled(!(index < 0));
}

void lmcSettingsDialog::txtBroadcast_textEdited(const QString& text) {
	ui.btnAddBroadcast->setEnabled(ipRegExp.exactMatch(text));
}

void lmcSettingsDialog::btnAddBroadcast_clicked(void) {
	QString address = ui.txtBroadcast->text();
	//	Do not add if not a valid ip address
	if(!ipRegExp.exactMatch(address))
		return;

	//	Check if the same address is already present in the list
	for(int index = 0; index < ui.lvBroadcasts->count(); index++) {
		QString text = ui.lvBroadcasts->item(index)->text();
		if(text.compare(address) == 0)
			return;
	}

	QListWidgetItem* item = new QListWidgetItem(ui.lvBroadcasts);
	item->setText(address);

	ui.txtBroadcast->clear();
	ui.btnAddBroadcast->setEnabled(false);
	ui.txtBroadcast->setFocus();
}

void lmcSettingsDialog::btnDeleteBroadcast_clicked(void) {
	if(ui.lvBroadcasts->currentRow() < 0)
		return;

	QListWidgetItem* item = ui.lvBroadcasts->takeItem(ui.lvBroadcasts->currentRow());
	delete item;
}

void lmcSettingsDialog::lvSounds_currentRowChanged(int index) {
	ui.btnPlaySound->setEnabled(!(index < 0));
	ui.btnSoundPath->setEnabled(!(index < 0));

	if(index < 0) {
		ui.txtSoundFile->clear();
		return;
	}

	QFileInfo fileInfo(ui.lvSounds->currentItem()->data(Qt::UserRole).toString());
	if(fileInfo.exists())
		ui.txtSoundFile->setText(fileInfo.fileName());
	else
		ui.txtSoundFile->setText(tr("<File Not Found>"));
}

void lmcSettingsDialog::btnPlaySound_clicked(void) {
	if(ui.lvSounds->currentRow() < 0)
		return;

	QSound::play(ui.lvSounds->currentItem()->data(Qt::UserRole).toString());
}

void lmcSettingsDialog::btnSoundPath_clicked(void) {
	if(ui.lvSounds->currentRow() < 0)
		return;

	QString soundPath = QFileDialog::getOpenFileName(this, tr("Select sound"),
		ui.lvSounds->currentItem()->data(Qt::UserRole).toString(), "Wave Files (*.wav)");
	if(!soundPath.isEmpty()) {
		ui.lvSounds->currentItem()->setData(Qt::UserRole, soundPath);
		lvSounds_currentRowChanged(ui.lvSounds->currentRow());
	}
}

void lmcSettingsDialog::btnResetSounds_clicked(void) {
	for(int index = 0; index < SE_Max; index++) {
		QListWidgetItem* pListItem = ui.lvSounds->item(index);
		pListItem->setData(Qt::UserRole, soundFile[index]);
	}
	lvSounds_currentRowChanged(ui.lvSounds->currentRow());
}

void lmcSettingsDialog::setPageHeaderStyle(QLabel* pLabel) {
    QFont font = pLabel->font();
    int fontSize = pLabel->fontInfo().pixelSize();
    fontSize += (fontSize * 0.2);
    font.setPixelSize(fontSize);
    font.setBold(true);
    pLabel->setFont(font);
}

void lmcSettingsDialog::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(tr("Preferences"));

	ui.chkAutoStart->setText(ui.chkAutoStart->text().arg(lmcStrings::appName()));
	ui.chkAutoShow->setText(ui.chkAutoShow->text().arg(lmcStrings::appName()));
	ui.lblFinePrint->setText(ui.lblFinePrint->text().arg(lmcStrings::appName()));

	if(!QSystemTrayIcon::isSystemTrayAvailable()) {
		ui.grpSysTray->setEnabled(false);
		ui.grpSysTray->setTitle(tr("System Tray (Not Available)"));
	}
	if(!QSystemTrayIcon::supportsMessages()) {
		ui.grpAlerts->setEnabled(false);
		ui.grpAlerts->setTitle(tr("Status Alerts (Not Available)"));
	}
    if(QAudioDeviceInfo::availableDevices(QAudio::AudioOutput).isEmpty()) {
		ui.grpSounds->setEnabled(false);
		ui.grpSounds->setTitle(tr("Sounds (Not Available)"));
	}

	for(int index = 0; index < ui.cboFontSize->count(); index++)
		ui.cboFontSize->setItemText(index, lmcStrings::fontSize()[index]);

	for(int index = 0; index < ui.lvSounds->count(); index++)
		ui.lvSounds->item(index)->setText(lmcStrings::soundDesc()[index]);

	for(int index = 0; index < ULV_Max; index++)
		ui.cboUserListView->setItemText(index, lmcStrings::userListView()[index]);

	QString updateLink = QString(IDA_DOMAIN"/downloads.php#translations");
	ui.lblUpdateLink->setText("<a href='" + updateLink + "'><span style='text-decoration: underline; color:#0000ff;'>" + 
		tr("Check for updates") + "</span></a>");

	cboTheme_currentIndexChanged(ui.cboTheme->currentIndex());

#ifdef Q_OS_MAC
	ui.rdbEnter->setText("Return");
	ui.rdbCmdEnter->setText(QString(QChar(0x2318)) + " + Return"); // U+2318 is the hex code for Bowen Knot symbol
#else
	ui.rdbEnter->setText("Enter");
	ui.rdbCmdEnter->setText("Ctrl + Enter");
#endif

	//	set minimum possible size
	layout()->setSizeConstraint(QLayout::SetMinimumSize);
}

void lmcSettingsDialog::loadSettings(void) {
    //	Auto start function not implemented on Mac since Mac itself provides an easy UI for it
#ifdef Q_OS_MAC
	ui.chkAutoStart->setChecked(false);
    ui.chkAutoStart->hide();
#else
	ui.chkAutoStart->setChecked(pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool());
#endif
	ui.chkAutoShow->setChecked(pSettings->value(IDS_AUTOSHOW, IDS_AUTOSHOW_VAL).toBool());
	ui.chkSysTray->setChecked(pSettings->value(IDS_SYSTRAY, IDS_SYSTRAY_VAL).toBool());
	ui.chkMinimizeTray->setChecked(pSettings->value(IDS_MINIMIZETRAY, IDS_MINIMIZETRAY_VAL).toBool());
	ui.chkSingleClickTray->setChecked(pSettings->value(IDS_SINGLECLICKTRAY, IDS_SINGLECLICKTRAY_VAL).toBool());
	ui.chkSysTrayMsg->setChecked(pSettings->value(IDS_SYSTRAYMSG, IDS_SYSTRAYMSG_VAL).toBool());
	ui.chkAllowSysTrayMin->setChecked(pSettings->value(IDS_ALLOWSYSTRAYMIN, IDS_ALLOWSYSTRAYMIN_VAL).toBool());
	QString langCode = pSettings->value(IDS_LANGUAGE, IDS_LANGUAGE_VAL).toString();
	for(int index = 0; index < ui.cboLanguage->count(); index ++) {
		QString code = ui.cboLanguage->itemData(index, Qt::UserRole).toString();
		if(langCode.compare(code) == 0) {
			ui.cboLanguage->setCurrentIndex(index);
			break;
		}
	}

	ui.txtUserName->setText(pSettings->value(IDS_USERNAME, IDS_USERNAME_VAL).toString());
	ui.txtFirstName->setText(pSettings->value(IDS_USERFIRSTNAME, IDS_USERFIRSTNAME_VAL).toString());
	ui.txtLastName->setText(pSettings->value(IDS_USERLASTNAME, IDS_USERLASTNAME_VAL).toString());
	ui.txtAbout->setPlainText(pSettings->value(IDS_USERABOUT, IDS_USERABOUT_VAL).toString());
	ui.spnRefreshTime->setValue(pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt());

	ui.rdbMessageTop->setChecked(pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool());
	ui.rdbMessageBottom->setChecked(!pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool());
	ui.chkPublicMessagePop->setChecked(pSettings->value(IDS_PUBMESSAGEPOP, IDS_PUBMESSAGEPOP_VAL).toBool());
	ui.chkEmoticon->setChecked(pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool());
	ui.chkMessageTime->setChecked(pSettings->value(IDS_MESSAGETIME, IDS_MESSAGETIME_VAL).toBool());
	ui.chkMessageDate->setChecked(pSettings->value(IDS_MESSAGEDATE, IDS_MESSAGEDATE_VAL).toBool());
	ui.chkAllowLinks->setChecked(pSettings->value(IDS_ALLOWLINKS, IDS_ALLOWLINKS_VAL).toBool());
	ui.chkPathToLink->setChecked(pSettings->value(IDS_PATHTOLINK, IDS_PATHTOLINK_VAL).toBool());
	ui.chkTrimMessage->setChecked(pSettings->value(IDS_TRIMMESSAGE, IDS_TRIMMESSAGE_VAL).toBool());
    ui.chkClearOnClose->setChecked(pSettings->value(IDS_CLEARONCLOSE, IDS_CLEARONCLOSE_VAL).toBool());
	font.fromString(pSettings->value(IDS_FONT, IDS_FONT_VAL).toString());
	color.setNamedColor(pSettings->value(IDS_COLOR, IDS_COLOR_VAL).toString());
	fontSize = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
	fontSize = qMin(FS_LARGE, qMax(FS_SMALL, fontSize));
	ui.cboFontSize->setCurrentIndex(fontSize);
	
	ui.chkHistory->setChecked(pSettings->value(IDS_HISTORY, IDS_HISTORY_VAL).toBool());
	ui.rdbSysHistoryPath->setChecked(pSettings->value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool());
	ui.rdbCustomHistoryPath->setChecked(!pSettings->value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool());
	ui.chkFileHistory->setChecked(pSettings->value(IDS_FILEHISTORY, IDS_FILEHISTORY_VAL).toBool());

	ui.chkAlert->setChecked(pSettings->value(IDS_ALERT, IDS_ALERT_VAL).toBool());
	ui.chkNoBusyAlert->setChecked(pSettings->value(IDS_NOBUSYALERT, IDS_NOBUSYALERT_VAL).toBool());
	ui.chkNoDNDAlert->setChecked(pSettings->value(IDS_NODNDALERT, IDS_NODNDALERT_VAL).toBool());
	ui.chkSound->setChecked(pSettings->value(IDS_SOUND, IDS_SOUND_VAL).toBool());
	// Check so that number of elements read from settings file does not exceed the number of elements
	// in the list view control. This prevents array out of bounds error.
	int size = qMin(pSettings->beginReadArray(IDS_SOUNDEVENTHDR), ui.lvSounds->count());
	for(int index = 0; index < size; index++) {
		pSettings->setArrayIndex(index);
		ui.lvSounds->item(index)->setCheckState((Qt::CheckState)pSettings->value(IDS_SOUNDEVENT).toInt());
	}
	pSettings->endArray();
	size = qMin(pSettings->beginReadArray(IDS_SOUNDFILEHDR), ui.lvSounds->count());
	for(int index = 0; index < size; index++) {
		pSettings->setArrayIndex(index);
		ui.lvSounds->item(index)->setData(Qt::UserRole, pSettings->value(IDS_SOUNDFILE).toString());
	}
	pSettings->endArray();
	ui.chkNoBusySound->setChecked(pSettings->value(IDS_NOBUSYSOUND, IDS_NOBUSYSOUND_VAL).toBool());
	ui.chkNoDNDSound->setChecked(pSettings->value(IDS_NODNDSOUND, IDS_NODNDSOUND_VAL).toBool());

	ui.spnTimeout->setValue(pSettings->value(IDS_TIMEOUT, IDS_TIMEOUT_VAL).toInt());
	ui.spnMaxRetries->setValue(pSettings->value(IDS_MAXRETRIES, IDS_MAXRETRIES_VAL).toInt());
	size = pSettings->beginReadArray(IDS_BROADCASTHDR);
	for(int index = 0; index < size; index++) {
		pSettings->setArrayIndex(index);
		QListWidgetItem* item = new QListWidgetItem(ui.lvBroadcasts);
		item->setText(pSettings->value(IDS_BROADCAST).toString());
	}
	pSettings->endArray();
	ui.txtMulticast->setText(pSettings->value(IDS_MULTICAST, IDS_MULTICAST_VAL).toString());
	ui.txtUDPPort->setText(pSettings->value(IDS_UDPPORT, IDS_UDPPORT_VAL).toString());
	ui.txtTCPPort->setText(pSettings->value(IDS_TCPPORT, IDS_TCPPORT_VAL).toString());

	ui.chkAutoFile->setChecked(pSettings->value(IDS_AUTOFILE, IDS_AUTOFILE_VAL).toBool());
	ui.chkAutoShowFile->setChecked(pSettings->value(IDS_AUTOSHOWFILE, IDS_AUTOSHOWFILE_VAL).toBool());
	ui.rdbFileTop->setChecked(pSettings->value(IDS_FILETOP, IDS_FILETOP_VAL).toBool());
	ui.rdbFileBottom->setChecked(!pSettings->value(IDS_FILETOP, IDS_FILETOP_VAL).toBool());
	ui.txtFilePath->setText(StdLocation::fileStorageDir());

	QString themePath = pSettings->value(IDS_THEME, IDS_THEME_VAL).toString();
	for(int index = 0; index < ui.cboTheme->count(); index ++) {
		QString theme = ui.cboTheme->itemData(index, Qt::UserRole).toString();
		if(themePath.compare(theme) == 0) {
			ui.cboTheme->setCurrentIndex(index);
			break;
		}
	}
	int userListView = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
	ui.cboUserListView->setCurrentIndex(userListView);
	ui.chkUserListToolTip->setChecked(pSettings->value(IDS_STATUSTOOLTIP, IDS_STATUSTOOLTIP_VAL).toBool());

	ui.rdbEnter->setChecked(!pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool());
	ui.rdbCmdEnter->setChecked(pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool());
}

void lmcSettingsDialog::saveSettings(void) {
	pSettings->setValue(IDS_VERSION, IDA_VERSION);

	pSettings->setValue(IDS_AUTOSTART, ui.chkAutoStart->isChecked(), IDS_AUTOSTART_VAL);
	pSettings->setValue(IDS_AUTOSHOW, ui.chkAutoShow->isChecked(), IDS_AUTOSHOW_VAL);
	pSettings->setValue(IDS_SYSTRAY, ui.chkSysTray->isChecked(), IDS_SYSTRAY_VAL);
	pSettings->setValue(IDS_MINIMIZETRAY, ui.chkMinimizeTray->isChecked(), IDS_MINIMIZETRAY_VAL);
	pSettings->setValue(IDS_SINGLECLICKTRAY, ui.chkSingleClickTray->isChecked(), IDS_SINGLECLICKTRAY_VAL);
	pSettings->setValue(IDS_SYSTRAYMSG, ui.chkSysTrayMsg->isChecked(), IDS_SYSTRAYMSG_VAL);
	pSettings->setValue(IDS_ALLOWSYSTRAYMIN, ui.chkAllowSysTrayMin->isChecked(), IDS_ALLOWSYSTRAYMIN_VAL);
	QString langCode = ui.cboLanguage->itemData(ui.cboLanguage->currentIndex(), Qt::UserRole).toString();
	pSettings->setValue(IDS_LANGUAGE, langCode, IDS_LANGUAGE_VAL);

	pSettings->setValue(IDS_USERNAME, ui.txtUserName->text(), IDS_USERNAME_VAL);
	pSettings->setValue(IDS_USERFIRSTNAME, ui.txtFirstName->text(), IDS_USERFIRSTNAME_VAL);
	pSettings->setValue(IDS_USERLASTNAME, ui.txtLastName->text(), IDS_USERLASTNAME_VAL);
	pSettings->setValue(IDS_USERABOUT, ui.txtAbout->toPlainText(), IDS_USERABOUT_VAL);
	pSettings->setValue(IDS_REFRESHTIME, ui.spnRefreshTime->value(), IDS_REFRESHTIME_VAL);

	pSettings->setValue(IDS_MESSAGETOP, ui.rdbMessageTop->isChecked(), IDS_MESSAGETOP_VAL);
	pSettings->setValue(IDS_PUBMESSAGEPOP, ui.chkPublicMessagePop->isChecked(), IDS_PUBMESSAGEPOP_VAL);
	pSettings->setValue(IDS_EMOTICON, ui.chkEmoticon->isChecked(), IDS_EMOTICON_VAL);
	pSettings->setValue(IDS_MESSAGETIME, ui.chkMessageTime->isChecked(), IDS_MESSAGETIME_VAL);
	pSettings->setValue(IDS_MESSAGEDATE, ui.chkMessageDate->isChecked(), IDS_MESSAGEDATE_VAL);
	pSettings->setValue(IDS_ALLOWLINKS, ui.chkAllowLinks->isChecked(), IDS_ALLOWLINKS_VAL);
	pSettings->setValue(IDS_PATHTOLINK, ui.chkPathToLink->isChecked(), IDS_PATHTOLINK_VAL);
	pSettings->setValue(IDS_TRIMMESSAGE, ui.chkTrimMessage->isChecked(), IDS_TRIMMESSAGE_VAL);
    pSettings->setValue(IDS_CLEARONCLOSE, ui.chkClearOnClose->isChecked(), IDS_CLEARONCLOSE_VAL);
	pSettings->setValue(IDS_FONT, font.toString(), IDS_FONT_VAL);
	pSettings->setValue(IDS_COLOR, color.name(), IDS_COLOR_VAL);
	pSettings->setValue(IDS_FONTSIZE, ui.cboFontSize->currentIndex(), IDS_FONTSIZE_VAL);

	pSettings->setValue(IDS_HISTORY, ui.chkHistory->isChecked(), IDS_HISTORY_VAL);
	pSettings->setValue(IDS_SYSHISTORYPATH, ui.rdbSysHistoryPath->isChecked(), IDS_SYSHISTORYPATH_VAL);
	pSettings->setValue(IDS_HISTORYPATH, ui.txtHistoryPath->text(), IDS_HISTORYPATH_VAL);
	pSettings->setValue(IDS_FILEHISTORY, ui.chkFileHistory->isChecked(), IDS_FILEHISTORY_VAL);

	pSettings->setValue(IDS_ALERT, ui.chkAlert->isChecked(), IDS_ALERT_VAL);
	pSettings->setValue(IDS_NOBUSYALERT, ui.chkNoBusyAlert->isChecked(), IDS_NOBUSYALERT_VAL);
	pSettings->setValue(IDS_NODNDALERT, ui.chkNoDNDAlert->isChecked(), IDS_NODNDALERT_VAL);
	pSettings->setValue(IDS_SOUND, ui.chkSound->isChecked(), IDS_SOUND_VAL);
	int checkCount = 0;
	int soundFileCount = 0;
	if(ui.lvSounds->count() > 0) {
		pSettings->beginWriteArray(IDS_SOUNDEVENTHDR);
		for(int index = 0; index < ui.lvSounds->count(); index++) {
			pSettings->setArrayIndex(index);
			pSettings->setValue(IDS_SOUNDEVENT, ui.lvSounds->item(index)->checkState());
			if(ui.lvSounds->item(index)->checkState() == IDS_SOUNDEVENT_VAL)
				checkCount++;
		}
		pSettings->endArray();
		pSettings->beginWriteArray(IDS_SOUNDFILEHDR);
		for(int index = 0; index < ui.lvSounds->count(); index++) {
			pSettings->setArrayIndex(index);
			pSettings->setValue(IDS_SOUNDFILE, ui.lvSounds->item(index)->data(Qt::UserRole).toString());
			if(ui.lvSounds->item(index)->data(Qt::UserRole).toString().compare(soundFile[index]) == 0)
				soundFileCount++;
		}
		pSettings->endArray();
	}
	if(ui.lvSounds->count() == 0 || checkCount == ui.lvSounds->count()) {
		pSettings->beginGroup(IDS_SOUNDEVENTHDR);
		pSettings->remove("");
		pSettings->endGroup();
	}
	if(ui.lvSounds->count() == 0 || soundFileCount == ui.lvSounds->count()) {
		pSettings->beginGroup(IDS_SOUNDFILEHDR);
		pSettings->remove("");
		pSettings->endGroup();
	}
	pSettings->setValue(IDS_NOBUSYSOUND, ui.chkNoBusySound->isChecked(), IDS_NOBUSYSOUND_VAL);
	pSettings->setValue(IDS_NODNDSOUND, ui.chkNoDNDSound->isChecked(), IDS_NODNDSOUND_VAL);

	pSettings->setValue(IDS_TIMEOUT, ui.spnTimeout->value(), IDS_TIMEOUT_VAL);
	pSettings->setValue(IDS_MAXRETRIES, ui.spnMaxRetries->value(), IDS_MAXRETRIES_VAL);
	//	If any broadcast address is specified, settings written to settings file
	//	Otherwise, the entire group is removed from the settings file
	if(ui.lvBroadcasts->count() > 0) {
		pSettings->beginWriteArray(IDS_BROADCASTHDR);
		for(int index = 0; index < ui.lvBroadcasts->count(); index++) {
			pSettings->setArrayIndex(index);
			pSettings->setValue(IDS_BROADCAST, ui.lvBroadcasts->item(index)->text());
		}
		pSettings->endArray();
	}
	if(ui.lvBroadcasts->count() == 0){
		pSettings->beginGroup(IDS_BROADCASTHDR);
		pSettings->remove("");
		pSettings->endGroup();
	}
	pSettings->setValue(IDS_MULTICAST, ui.txtMulticast->text(), IDS_MULTICAST_VAL);
	pSettings->setValue(IDS_UDPPORT, ui.txtUDPPort->text(), IDS_UDPPORT_VAL);
	pSettings->setValue(IDS_TCPPORT, ui.txtTCPPort->text(), IDS_TCPPORT_VAL);

	pSettings->setValue(IDS_AUTOFILE, ui.chkAutoFile->isChecked(), IDS_AUTOFILE_VAL);
	pSettings->setValue(IDS_AUTOSHOWFILE, ui.chkAutoShowFile->isChecked(), IDS_AUTOSHOWFILE_VAL);
	pSettings->setValue(IDS_FILETOP, ui.rdbFileTop->isChecked(), IDS_FILETOP_VAL);
	pSettings->setValue(IDS_FILESTORAGEPATH, ui.txtFilePath->text(), IDS_FILESTORAGEPATH_VAL);

	QString themePath = ui.cboTheme->itemData(ui.cboTheme->currentIndex(), Qt::UserRole).toString();
	pSettings->setValue(IDS_THEME, themePath, IDS_THEME_VAL);
	pSettings->setValue(IDS_USERLISTVIEW, ui.cboUserListView->currentIndex(), IDS_USERLISTVIEW_VAL);
	pSettings->setValue(IDS_STATUSTOOLTIP, ui.chkUserListToolTip->isChecked(), IDS_STATUSTOOLTIP_VAL);

	pSettings->setValue(IDS_SENDKEYMOD, ui.rdbCmdEnter->isChecked(), IDS_SENDKEYMOD_VAL);

	pSettings->sync();
}
