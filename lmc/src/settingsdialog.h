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


#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QFileDialog>
#include <QDesktopServices>
#include "ui_settingsdialog.h"
#include "shared.h"
#include "settings.h"
#include "history.h"
#include "filetransfer.h"
#include "application.h"

class lmcSettingsDialog : public QDialog {
	Q_OBJECT

public:
	lmcSettingsDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
	~lmcSettingsDialog(void);

	void init(void);
	void settingsChanged(void);

signals:
	void historyCleared(void);
	void fileHistoryCleared(void);

protected:
	void changeEvent(QEvent* pEvent);

private slots:
	void lvCategories_currentRowChanged(int currentRow);
	void btnOk_clicked(void);
	void chkMessageTime_toggled(bool checked);
	void rdbFontSize_toggled(bool checked);
	void rdbSysHistoryPath_toggled(bool checked);
	void btnHistoryPath_clicked(void);
	void btnFilePath_clicked(void);
	void btnClearHistory_clicked(void);
	void btnClearFileHistory_clicked(void);
	void chkSound_toggled(bool checked);
	void chkAutoShowFile_toggled(bool checked);
	void btnViewFiles_clicked(void);

private:
	void setUIText(void);
	void loadSettings(void);
	void saveSettings(void);

	Ui::SettingsDialog ui;
	lmcSettings* pSettings;
	int fontSize;
	QIntValidator* pPortValidator;
};

#endif // SETTINGSDIALOG_H