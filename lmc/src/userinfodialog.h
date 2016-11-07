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


#ifndef USERINFODIALOG_H
#define USERINFODIALOG_H

#include <QDialog>
#include "ui_userinfodialog.h"
#include "shared.h"
#include "settings.h"
#include "stdlocation.h"
#include "xmlmessage.h"

class lmcUserInfoDialog : public QDialog
{
	Q_OBJECT

public:
	lmcUserInfoDialog(QWidget *parent = 0);
	~lmcUserInfoDialog();

	void init(void);
	void setInfo(XmlMessage* pMessage);
	void settingsChanged(void);

protected:
	void changeEvent(QEvent* pEvent);

private:
	void setUIText(void);
	void fillFields(void);

	Ui::UserInfoDialog ui;
	lmcSettings* pSettings;
	XmlMessage userInfo;
};

#endif // USERINFODIALOG_H
