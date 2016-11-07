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


#include <QDesktopWidget>
#include "helpwindow.h"

lmcHelpWindow::lmcHelpWindow(QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);
	QRect scr = QApplication::desktop()->screenGeometry();
	move(scr.center() - rect().center());
}

lmcHelpWindow::~lmcHelpWindow() {
}

void lmcHelpWindow::init(void) {
	setWindowIcon(QIcon(IDR_APPICON));

	pSettings = new lmcSettings();
	restoreGeometry(pSettings->value(IDS_WINDOWHELP).toByteArray());
	setUIText();
}

void lmcHelpWindow::stop(void) {
	pSettings->setValue(IDS_WINDOWHELP, saveGeometry());
}

void lmcHelpWindow::settingsChanged(void) {
}

void lmcHelpWindow::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
    default:
        break;
	}

	QWidget::changeEvent(pEvent);
}

void lmcHelpWindow::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(tr("Help"));

	QString html = tr(
		"<html>"
			"<head>"
				"<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />"
			"</head>"
			"<body>"
				"<h2>Welcome to LAN Messenger Help</h2>"
				"<h3>Help</h3>"
				"<p>"
				"You can find documentation on LAN Messenger's <a href='http://sourceforge.net/p/lanmsngr/wiki/Manual/'>wiki</a> website."
				"</p>"
				"<p>"
				"Before asking any question, please refer yourself to the <a href='http://sourceforge.net/p/lanmsngr/wiki/FAQ/'>FAQ</a>."
				"</p>"
				"<p>"
				"You might then get (and give) help on the <a href='http://sourceforge.net/projects/lanmsngr/support'>Support Forums</a>."
				"</p>"
				"<h3>Contribute to the project</h3>"
				"<p>"
				"You can help the LAN Messenger project giving some of your time to help the community, to translate the application or documentation, and to test the program. And of course, you can <b>promote</b> LAN Messenger."
				"</p>"
			"</body>"
		"</html>");
	ui.txtHelp->setHtml(html);
}
