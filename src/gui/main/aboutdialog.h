// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QPicture>
#include "ui_aboutdialog.h"
#include "core/shared.h"
#include "core/settings.h"

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

#endif // ABOUTDIALOG_H
