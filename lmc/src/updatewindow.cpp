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
#include "updatewindow.h"
#include "ui_updatewindow.h"
#include "shared.h"

lmcUpdateWindow::lmcUpdateWindow(QRect* pRect, QWidget *parent) : QWidget(parent), ui(new Ui::UpdateWindow) {
	ui->setupUi(this);
	//	set fixed size
	layout()->setSizeConstraint(QLayout::SetFixedSize);

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

	connect(ui->btnRecheck, SIGNAL(clicked()), this, SLOT(btnRecheck_clicked()));

    ui->btnClose->installEventFilter(this);
    ui->btnRecheck->installEventFilter(this);

	webVersion = QString::null;
}

lmcUpdateWindow::~lmcUpdateWindow() {
    delete ui;
}

void lmcUpdateWindow::init(void) {
	setWindowIcon(QIcon(IDR_APPICON));
	ui->lblOutput->setBackgroundRole(QPalette::Base);
	ui->lblOutput->setAutoFillBackground(true);

	setUIText();
	checkForUpdates();
}

void lmcUpdateWindow::stop(void) {
}

void lmcUpdateWindow::receiveMessage(MessageType type, QString *lpszUserId, XmlMessage *pMessage) {
	Q_UNUSED(lpszUserId);

	switch(type) {
	case MT_Version:
		webVersion = pMessage->data(XN_VERSION);
		if(Helper::compareVersions(webVersion, QString(IDA_VERSION)) > 0)
			status = US_New;	// newer version available online
		else
			status = US_Latest;
		setUIText();
		break;
	case MT_WebFailed:
		status = US_Error;
		setUIText();
		break;
	default:
		break;
	}

	ui->btnRecheck->setEnabled(true);
}

void lmcUpdateWindow::settingsChanged(void) {
}

bool lmcUpdateWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
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

void lmcUpdateWindow::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
	default:
		break;
	}

	QWidget::changeEvent(pEvent);
}

void lmcUpdateWindow::btnRecheck_clicked(void) {
	checkForUpdates();
}

void lmcUpdateWindow::setUIText(void) {
	ui->retranslateUi(this);

	QString title = tr("%1 updates");
	setWindowTitle(title.arg(lmcStrings::appName()));

	ui->lblOutput->setText(getStatusMessage());
}

void lmcUpdateWindow::checkForUpdates() {
	ui->btnRecheck->setEnabled(false);
	status = US_Check;
	setUIText();

	emit messageSent(MT_Version, NULL, NULL);
}

QString lmcUpdateWindow::getStatusMessage(void) {
	QString message;

	switch(status) {
	case US_Check:
		message = tr("Launching an update request...");
		break;
	case US_Error:
		message = tr("An error occured while checking for updates.");
		break;
	case US_New:
		message = tr("The new version %1 is available on %2 web site.").arg(webVersion, lmcStrings::appName());
		break;
	case US_Latest:
		message = tr("You have the latest version of %1.").arg(lmcStrings::appName());
		break;
	}

	return message;
}
