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


#include "userinfowindow.h"

lmcUserInfoWindow::lmcUserInfoWindow(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	//	set fixed size
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	//	remove the help button from window button group
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	//	Destroy the window when it closes
	setAttribute(Qt::WA_DeleteOnClose, true);
}

lmcUserInfoWindow::~lmcUserInfoWindow() {
}

void lmcUserInfoWindow::init(void) {
	setWindowIcon(QIcon(IDR_APPICON));

	QFont font = ui.lblUserName->font();
	int fontSize = ui.lblUserName->fontInfo().pixelSize();
	fontSize += (fontSize * 0.1);
	font.setPixelSize(fontSize);
	font.setBold(true);
	ui.lblUserName->setFont(font);

	pSettings = new lmcSettings();
	setUIText();
}

void lmcUserInfoWindow::setInfo(XmlMessage* pMessage) {
	userInfo = *pMessage;
	setUIText();
	ui.tabWidget->setCurrentIndex(0);
}

void lmcUserInfoWindow::settingsChanged(void) {
}

void lmcUserInfoWindow::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QDialog::changeEvent(pEvent);
}

void lmcUserInfoWindow::setUIText(void) {
	ui.retranslateUi(this);
	setWindowTitle(tr("User Information"));

	if(userInfo.header(XN_TYPE).isNull())
		return;

    QString filePath = userInfo.data(XN_AVATAR);
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
