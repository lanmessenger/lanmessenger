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


#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QDesktopServices>
#include "ui_settingsdialog.h"
#include "shared.h"
#include "settings.h"
#include "history.h"
#include "stdlocation.h"
#include "application.h"
#include "messagelog.h"
#include "theme.h"
#include "soundplayer.h"

class lmcSettingsDialog : public QDialog {
	Q_OBJECT

public:
    lmcSettingsDialog(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
	~lmcSettingsDialog(void);

	void init(void);
	void settingsChanged(void);

signals:
	void historyCleared(void);
	void fileHistoryCleared(void);

protected:
	void changeEvent(QEvent* pEvent);
    void timerEvent(QTimerEvent *event);

private slots:
	void lvCategories_currentRowChanged(int currentRow);
	void btnOk_clicked(void);
	void chkMessageTime_toggled(bool checked);
	void chkAllowLinks_toggled(bool checked);
	void rdbSysHistoryPath_toggled(bool checked);
	void btnHistoryPath_clicked(void);
	void btnFilePath_clicked(void);
	void btnClearHistory_clicked(void);
	void btnClearFileHistory_clicked(void);
	void chkSound_toggled(bool checked);
	void chkAutoShowFile_toggled(bool checked);
	void btnViewFiles_clicked(void);
	void btnFont_clicked(void);
	void btnColor_clicked(void);
	void btnReset_clicked(void);
	void cboTheme_currentIndexChanged(int index);
	void lvBroadcasts_currentRowChanged(int index);
	void txtBroadcast_textEdited(const QString& text);
	void btnAddBroadcast_clicked(void);
	void btnDeleteBroadcast_clicked(void);
	void lvSounds_currentRowChanged(int index);
	void btnPlaySound_clicked(void);
	void btnSoundPath_clicked(void);
	void btnResetSounds_clicked(void);
    void btnRefreshTheme_clicked(void);

private:
    void setPageHeaderStyle(QLabel* pLabel);
	void setUIText(void);
	void loadSettings(void);
	void saveSettings(void);
    void insertStatusMessageinLog();

	Ui::SettingsDialog ui;
	lmcSettings* pSettings;
	int fontSize;
	QFont font;
	QColor color;
	QIntValidator* pPortValidator;
	QRegExp	ipRegExp;
	QRegExpValidator* pIpValidator;
	lmcMessageLog* pMessageLog;
    int statusTimerId;
    int statusNow;
};

#endif // SETTINGSDIALOG_H
