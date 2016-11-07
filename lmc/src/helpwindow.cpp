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


#include <QDesktopWidget>
#include "helpwindow.h"

lmcHelpWindow::lmcHelpWindow(QRect* pRect, QWidget *parent) : QWidget(parent) {
	ui.setupUi(this);

	//	Destroy the window when it closes
	setAttribute(Qt::WA_DeleteOnClose, true);

	move(pRect->center() - rect().center());
	QRect screenRect = QApplication::desktop()->screenGeometry();
	if(!screenRect.contains(geometry(), true)) {
		QRect windowRect = geometry();
		if(windowRect.right() > screenRect.right())
			windowRect.translate(screenRect.right() - windowRect.right(), 0);
		if(windowRect.left() < screenRect.left())
			windowRect.translate(qAbs(windowRect.left() - screenRect.left()), 0);
		if(windowRect.bottom() > screenRect.bottom())
			windowRect.translate(0, screenRect.bottom() - windowRect.bottom());
		if(windowRect.top() < screenRect.top())
			windowRect.translate(0, qAbs(windowRect.top() - screenRect.top()));
		setGeometry(windowRect);
	}

    ui.txtHelp->installEventFilter(this);
    ui.btnClose->installEventFilter(this);
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

bool lmcHelpWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
    Q_UNUSED(pObject);
    if(pEvent->type() == QEvent::KeyPress) {
        QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
        if(pKeyEvent->key() == Qt::Key_Escape) {
            close();
            return true;
        }
    }

    return false;
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

	QString helpLink = QString(IDA_DOMAIN"/help.php");
	QString faqLink = QString(IDA_DOMAIN"/faq.php");
	QString supportLink = QString(IDA_DOMAIN"/support.php");

	QString html = tr(
		"<html>"
			"<head>"
				"<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />"
			"</head>"
			"<body>"
				"<h2>Welcome to LAN Messenger Help</h2>"
				"<h3>Help</h3>"
				"<p>"
				"You can find documentation on LAN Messenger's <a href='%1'>wiki</a> website."
				"</p>"
				"<p>"
				"Before asking any question, please refer yourself to the <a href='%2'>FAQ</a>."
				"</p>"
				"<p>"
				"You might then get (and give) help on the <a href='%3'>Support Forums</a>."
				"</p>"
				"<h3>Contribute to the project</h3>"
				"<p>"
				"You can help the LAN Messenger project giving some of your time to help the community, to translate the application or documentation, and to test the program. And of course, you can <b>promote</b> LAN Messenger."
				"</p>"
			"</body>"
		"</html>");
	ui.txtHelp->setHtml(html.arg(helpLink, faqLink, supportLink));
}
