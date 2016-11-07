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


#ifndef USERINFOWINDOW_H
#define USERINFOWINDOW_H

#include <QDialog>
#include "ui_userinfowindow.h"
#include "shared.h"
#include "settings.h"
#include "stdlocation.h"
#include "xmlmessage.h"

class lmcUserInfoWindow : public QDialog
{
	Q_OBJECT

public:
	lmcUserInfoWindow(QWidget *parent = 0);
	~lmcUserInfoWindow();

	void init(void);
	void setInfo(XmlMessage* pMessage);
	void settingsChanged(void);

protected:
	void changeEvent(QEvent* pEvent);

private:
	void setUIText(void);
	void fillFields(void);

    Ui::UserInfoWindow ui;
	lmcSettings* pSettings;
	XmlMessage userInfo;
};

#endif // USERINFOWINDOW_H
