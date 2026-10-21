// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <qevent.h>
#include "ui_helpwindow.h"
#include "core/shared.h"
#include "core/settings.h"

class lmcHelpWindow : public QWidget {
  Q_OBJECT

public:
  lmcHelpWindow(QRect* pRect, QWidget* parent = 0);
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
