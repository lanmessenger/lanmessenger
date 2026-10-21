// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QListWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QDesktopServices>
#include "ui_settingsdialog.h"
#include "core/shared.h"
#include "core/settings.h"
#include "messaging/history.h"
#include "core/stdlocation.h"
#include "app/application.h"
#include "gui/chat/messagelog.h"
#include "core/theme.h"
#include "core/soundplayer.h"

class lmcSettingsDialog : public QDialog {
  Q_OBJECT

public:
  lmcSettingsDialog(QWidget* parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~lmcSettingsDialog(void);

  void init(void);
  void settingsChanged(void);

signals:
  void historyCleared(void);
  void fileHistoryCleared(void);

protected:
  void changeEvent(QEvent* pEvent);
  void timerEvent(QTimerEvent* event);

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
  QRegularExpression ipRegExp;
  QRegularExpressionValidator* pIpValidator;
  lmcMessageLog* pMessageLog;
  int statusTimerId;
  int statusNow;
};

#endif // SETTINGSDIALOG_H
