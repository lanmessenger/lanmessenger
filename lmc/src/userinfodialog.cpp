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

void lmcUserInfoDialog::setInfo(XmlMessage* pMessage) {
	userInfo = *pMessage;
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

	if(userInfo.header(XN_TYPE).isNull())
		return;

	QDir cacheDir(StdLocation::cacheDir());
	QString fileName = "avt_" + userInfo.data(XN_USERID) + ".png";
	QString filePath = cacheDir.absoluteFilePath(fileName);
	//	if image not found, use the default avatar image for this user
	if(!QFile::exists(filePath))
		filePath = AVT_DEFAULT;
	ui.lblAvatar->setPixmap(QPixmap(filePath));
	ui.lblUserName->setText(userInfo.data(XN_NAME));
	ui.lblStatus->setText(lmcStrings::statusDesc()[Helper::statusIndexFromCode(userInfo.data(XN_STATUS))]);

	QString data = userInfo.data(XN_FIRSTNAME);
	if(!data.isNull() && data.compare("N/A") != 0)
		ui.txtFirstName->setText(data);
	data = userInfo.data(XN_LASTNAME);
	if(!data.isNull() && data.compare("N/A") != 0)
		ui.txtLastName->setText(data);
	data = userInfo.data(XN_ABOUT);
	if(!data.isNull() && data.compare("N/A") != 0)
		ui.txtAbout->setPlainText(data);

	ui.lblIPAddress->setText(userInfo.data(XN_ADDRESS));
	ui.lblLogonName->setText(userInfo.data(XN_LOGON));
	ui.lblComputerName->setText(userInfo.data(XN_HOST));
	ui.lblOSName->setText(userInfo.data(XN_OS));
	ui.lblVersion->setText(userInfo.data(XN_VERSION));
}
