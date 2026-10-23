// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QDialog>
#include <QString>
#include <QFont>
#include <QColor>
#include <QRegularExpression>
#include "ui_settingsdialog.h"

class lmcSettings;
class ChatMessageView;
class QLabel;
class QIntValidator;
class QRegularExpressionValidator;
class QEvent;
class QTimerEvent;

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
  void refreshPreview(void);
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
  void populateThemes(void);
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
  ChatMessageView* pMessageLog;
  int statusTimerId;
  int statusNow;
};
