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


#include <QUrl>
#include <QSound>
#include <QSystemTrayIcon>
#include <QLocale>
#include <QMessageBox>
#include "settingsdialog.h"

lmcSettingsDialog::lmcSettingsDialog(QWidget *parent, Qt::WFlags flags) : QDialog(parent, flags) {
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
		pListItem->setCheckState(Qt::Checked);
	}

	Themes themes = lmcTheme::availableThemes();
	for(int index = 0; index < themes.count(); index++)
		ui.cboTheme->addItem(themes.at(index).name, themes.at(index).path);

	fontSize = 0;
	font = QApplication::font();
	color = QApplication::palette().text().color();
	ui.lvCategories->setCurrentRow(0);
	init();
}

lmcSettingsDialog::~lmcSettingsDialog(void) {
}

void lmcSettingsDialog::init(void) {
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

	pPortValidator = new QIntValidator(1, 65535, this);
	ui.txtUDPPort->setValidator(pPortValidator);
	ui.txtTCPPort->setValidator(pPortValidator);

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

	if(checked) {
		ui.txtHistoryPath->setText(History::historyFile());
	}
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
    if(!QSound::isAvailable()) {
		ui.grpSounds->setEnabled(false);
		ui.grpSounds->setTitle(tr("Sounds (Not Available)"));
	}

	for(int index = 0; index < ui.cboFontSize->count(); index++)
		ui.cboFontSize->setItemText(index, lmcStrings::fontSize()[index]);

	for(int index = 0; index < ui.lvSounds->count(); index++)
		ui.lvSounds->item(index)->setText(lmcStrings::soundDesc()[index]);

	QString updateLink = QString(IDA_DOMAIN"/downloads.php#translations");
	ui.lblUpdateLink->setText("<a href='" + updateLink + "'><span style='text-decoration: underline; color:#0000ff;'>" + 
		tr("Check for updates") + "</span></a>");

	cboTheme_currentIndexChanged(ui.cboTheme->currentIndex());

#ifdef Q_WS_MAC
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
	//	Auto start function not implemented on Mac
#ifdef Q_WS_MAC
	ui.chkAutoStart->setChecked(false);
	ui.chkAutoStart->setEnabled(false);
#else
	ui.chkAutoStart->setChecked(pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool());
#endif
	ui.chkAutoShow->setChecked(pSettings->value(IDS_AUTOSHOW, IDS_AUTOSHOW_VAL).toBool());
	ui.chkSysTray->setChecked(pSettings->value(IDS_SYSTRAY, IDS_SYSTRAY_VAL).toBool());
	ui.chkMinimizeTray->setChecked(pSettings->value(IDS_MINIMIZETRAY, IDS_MINIMIZETRAY_VAL).toBool());
	ui.chkSingleClickTray->setChecked(pSettings->value(IDS_SINGLECLICKTRAY, IDS_SINGLECLICKTRAY_VAL).toBool());
	ui.chkSysTrayMsg->setChecked(pSettings->value(IDS_SYSTRAYMSG, IDS_SYSTRAYMSG_VAL).toBool());
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

	ui.chkEmoticon->setChecked(pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool());
	ui.chkMessageTime->setChecked(pSettings->value(IDS_MESSAGETIME, IDS_MESSAGETIME_VAL).toBool());
	ui.chkMessageDate->setChecked(pSettings->value(IDS_MESSAGEDATE, IDS_MESSAGEDATE_VAL).toBool());
	ui.chkAllowLinks->setChecked(pSettings->value(IDS_ALLOWLINKS, IDS_ALLOWLINKS_VAL).toBool());
	ui.chkPathToLink->setChecked(pSettings->value(IDS_PATHTOLINK, IDS_PATHTOLINK_VAL).toBool());
	ui.chkTrimMessage->setChecked(pSettings->value(IDS_TRIMMESSAGE, IDS_TRIMMESSAGE_VAL).toBool());
	ui.rdbMessageTop->setChecked(pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool());
	ui.rdbMessageBottom->setChecked(!pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool());
	font.fromString(pSettings->value(IDS_FONT, IDS_FONT_VAL).toString());
	color.setNamedColor(pSettings->value(IDS_COLOR, IDS_COLOR_VAL).toString());
	fontSize = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
	fontSize = qMin(FS_LARGE, qMax(FS_SMALL, fontSize));
	ui.cboFontSize->setCurrentIndex(fontSize);
	
	ui.chkHistory->setChecked(pSettings->value(IDS_HISTORY, IDS_HISTORY_VAL).toBool());
	ui.rdbSysHistoryPath->setChecked(pSettings->value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool());
	ui.rdbCustomHistoryPath->setChecked(!pSettings->value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool());
	ui.txtHistoryPath->setText(History::historyFile());
	ui.chkFileHistory->setChecked(pSettings->value(IDS_FILEHISTORY, IDS_FILEHISTORY_VAL).toBool());

	ui.chkAlert->setChecked(pSettings->value(IDS_ALERT, IDS_ALERT_VAL).toBool());
	ui.chkNoBusyAlert->setChecked(pSettings->value(IDS_NOBUSYALERT, IDS_NOBUSYALERT_VAL).toBool());
	ui.chkNoDNDAlert->setChecked(pSettings->value(IDS_NODNDALERT, IDS_NODNDALERT_VAL).toBool());
	ui.chkSound->setChecked(pSettings->value(IDS_SOUND, IDS_SOUND_VAL).toBool());
	int size = pSettings->beginReadArray(IDS_SOUNDEVENTHDR);
	for(int index = 0; index < size; index++) {
		pSettings->setArrayIndex(index);
		ui.lvSounds->item(index)->setCheckState((Qt::CheckState)pSettings->value(IDS_EVENT).toInt());
	}
	pSettings->endArray();
	ui.chkNoBusySound->setChecked(pSettings->value(IDS_NOBUSYSOUND, IDS_NOBUSYSOUND_VAL).toBool());
	ui.chkNoDNDSound->setChecked(pSettings->value(IDS_NODNDSOUND, IDS_NODNDSOUND_VAL).toBool());

	ui.spnTimeout->setValue(pSettings->value(IDS_TIMEOUT, IDS_TIMEOUT_VAL).toInt());
	ui.spnMaxRetries->setValue(pSettings->value(IDS_MAXRETRIES, IDS_MAXRETRIES_VAL).toInt());
	ui.txtBroadcast->setText(pSettings->value(IDS_BROADCAST, IDS_BROADCAST_VAL).toString());
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

	ui.rdbEnter->setChecked(!pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool());
	ui.rdbCmdEnter->setChecked(pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool());
}

void lmcSettingsDialog::saveSettings(void) {
	pSettings->setValue(IDS_VERSION, IDA_VERSION);

	pSettings->setValue(IDS_AUTOSTART, ui.chkAutoStart->isChecked());
	pSettings->setValue(IDS_AUTOSHOW, ui.chkAutoShow->isChecked());
	pSettings->setValue(IDS_SYSTRAY, ui.chkSysTray->isChecked());
	pSettings->setValue(IDS_MINIMIZETRAY, ui.chkMinimizeTray->isChecked());
	pSettings->setValue(IDS_SINGLECLICKTRAY, ui.chkSingleClickTray->isChecked());
	pSettings->setValue(IDS_SYSTRAYMSG, ui.chkSysTrayMsg->isChecked());
	QString langCode = ui.cboLanguage->itemData(ui.cboLanguage->currentIndex(), Qt::UserRole).toString();
	pSettings->setValue(IDS_LANGUAGE, langCode);

	pSettings->setValue(IDS_USERNAME, ui.txtUserName->text());
	pSettings->setValue(IDS_USERFIRSTNAME, ui.txtFirstName->text());
	pSettings->setValue(IDS_USERLASTNAME, ui.txtLastName->text());
	pSettings->setValue(IDS_USERABOUT, ui.txtAbout->toPlainText());
	pSettings->setValue(IDS_REFRESHTIME, ui.spnRefreshTime->value());

	pSettings->setValue(IDS_EMOTICON, ui.chkEmoticon->isChecked());
	pSettings->setValue(IDS_MESSAGETIME, ui.chkMessageTime->isChecked());
	pSettings->setValue(IDS_MESSAGEDATE, ui.chkMessageDate->isChecked());
	pSettings->setValue(IDS_ALLOWLINKS, ui.chkAllowLinks->isChecked());
	pSettings->setValue(IDS_PATHTOLINK, ui.chkPathToLink->isChecked());
	pSettings->setValue(IDS_TRIMMESSAGE, ui.chkTrimMessage->isChecked());
	pSettings->setValue(IDS_MESSAGETOP, ui.rdbMessageTop->isChecked());
	pSettings->setValue(IDS_FONT, font.toString());
	pSettings->setValue(IDS_COLOR, color.name());
	pSettings->setValue(IDS_FONTSIZE, ui.cboFontSize->currentIndex());

	pSettings->setValue(IDS_HISTORY, ui.chkHistory->isChecked());
	pSettings->setValue(IDS_SYSHISTORYPATH, ui.rdbSysHistoryPath->isChecked());
	pSettings->setValue(IDS_HISTORYPATH, ui.txtHistoryPath->text());
	pSettings->setValue(IDS_FILEHISTORY, ui.chkFileHistory->isChecked());

	pSettings->setValue(IDS_ALERT, ui.chkAlert->isChecked());
	pSettings->setValue(IDS_NOBUSYALERT, ui.chkNoBusyAlert->isChecked());
	pSettings->setValue(IDS_NODNDALERT, ui.chkNoDNDAlert->isChecked());
	pSettings->setValue(IDS_SOUND, ui.chkSound->isChecked());
	pSettings->beginWriteArray(IDS_SOUNDEVENTHDR);
	for(int index = 0; index < ui.lvSounds->count(); index++) {
		pSettings->setArrayIndex(index);
		pSettings->setValue(IDS_EVENT, ui.lvSounds->item(index)->checkState());
	}
	pSettings->endArray();
	pSettings->setValue(IDS_NOBUSYSOUND, ui.chkNoBusySound->isChecked());
	pSettings->setValue(IDS_NODNDSOUND, ui.chkNoDNDSound->isChecked());

	pSettings->setValue(IDS_TIMEOUT, ui.spnTimeout->value());
	pSettings->setValue(IDS_MAXRETRIES, ui.spnMaxRetries->value());
	pSettings->setValue(IDS_BROADCAST, ui.txtBroadcast->text());
	pSettings->setValue(IDS_UDPPORT, ui.txtUDPPort->text());
	pSettings->setValue(IDS_TCPPORT, ui.txtTCPPort->text());

	pSettings->setValue(IDS_AUTOFILE, ui.chkAutoFile->isChecked());
	pSettings->setValue(IDS_AUTOSHOWFILE, ui.chkAutoShowFile->isChecked());
	pSettings->setValue(IDS_FILETOP, ui.rdbFileTop->isChecked());
	pSettings->setValue(IDS_FILESTORAGEPATH, ui.txtFilePath->text());

	QString themePath = ui.cboTheme->itemData(ui.cboTheme->currentIndex(), Qt::UserRole).toString();
	pSettings->setValue(IDS_THEME, themePath);

	pSettings->setValue(IDS_SENDKEYMOD, ui.rdbCmdEnter->isChecked());

	pSettings->sync();
}
