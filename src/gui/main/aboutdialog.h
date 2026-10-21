// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QDialog>
#include "ui_aboutdialog.h"

class lmcSettings;
class QEvent;

class lmcAboutDialog : public QDialog {
  Q_OBJECT

public:
  lmcAboutDialog(QWidget* parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~lmcAboutDialog(void);

  void init(void);
  void settingsChanged(void);

protected:
  void changeEvent(QEvent* pEvent);

private:
  void setUIText(void);

  Ui::AboutDialog ui;
  lmcSettings* pSettings;
};
