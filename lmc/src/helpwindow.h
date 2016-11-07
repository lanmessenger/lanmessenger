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


#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <QWidget>
#include <qevent.h>
#include "ui_helpwindow.h"
#include "shared.h"
#include "settings.h"

class lmcHelpWindow : public QWidget
{
	Q_OBJECT

public:
	lmcHelpWindow(QRect* pRect, QWidget *parent = 0);
	~lmcHelpWindow();

	void init(void);
	void stop(void);
	void settingsChanged(void);

protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent);
	void changeEvent(QEvent* pEvent);

private:
	void setUIText(void);

	Ui::HelpWindow ui;
	lmcSettings* pSettings;
};

#endif // HELPWINDOW_H
