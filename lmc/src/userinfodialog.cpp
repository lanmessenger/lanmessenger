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


#include "userinfodialog.h"

lmcUserInfoDialog::lmcUserInfoDialog(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

lmcUserInfoDialog::~lmcUserInfoDialog() {
}

void lmcUserInfoDialog::init(void) {
	setWindowIcon(QIcon(IDR_APPICON));

	pSettings = new lmcSettings();
	setUIText();
}

void lmcUserInfoDialog::setInfo(QString* lpszUserInfo) {
	userInfo = lpszUserInfo->split(DELIMITER, QString::SkipEmptyParts);
	setUIText();
	ui.tabWidget->setCurrentIndex(0);
}

void lmcUserInfoDialog::settingsChanged(void) {
}

void lmcUserInfoDialog::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QDialog::changeEvent(pEvent);
}

void lmcUserInfoDialog::setUIText(void) {
	ui.retranslateUi(this);
	setWindowTitle(tr("User Information"));

	if(userInfo.isEmpty())
		return;

	ui.lblAvatar->setPixmap(QPixmap(avtPic[userInfo[UD_Avatar].toInt()]));
	ui.lblUserName->setText(userInfo[UD_Name]);
	ui.lblStatus->setText(lmcStrings::statusDesc()[Helper::statusIndexFromCode(userInfo[UD_Status])]);

	// in case the remote user does not send this info, skip this block
	if(userInfo.count() > UD_FirstName) {
		if(userInfo[UD_FirstName].compare("N/A") != 0)
			ui.txtFirstName->setText(userInfo[UD_FirstName]);
		if(userInfo[UD_LastName].compare("N/A") != 0)
			ui.txtLastName->setText(userInfo[UD_LastName]);
		if(userInfo[UD_About].compare("N/A") != 0)
			ui.txtAbout->setPlainText(userInfo[UD_About]);
	}

	ui.lblIPAddress->setText(userInfo[UD_Address]);
	ui.lblLogonName->setText(userInfo[UD_Logon]);
	ui.lblComputerName->setText(userInfo[UD_Host]);
	ui.lblOSName->setText(userInfo[UD_OS]);
	ui.lblVersion->setText(userInfo[UD_Version]);
}
