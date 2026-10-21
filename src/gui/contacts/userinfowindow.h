// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QDialog>
#include "ui_userinfowindow.h"
#include "core/shared.h"
#include "core/settings.h"
#include "core/stdlocation.h"
#include "messaging/xmlmessage.h"

class lmcUserInfoWindow : public QDialog {
  Q_OBJECT

public:
  lmcUserInfoWindow(QWidget* parent = 0);
  ~lmcUserInfoWindow();

  void init(void);
  void setInfo(XmlMessage* pMessage);
  void settingsChanged(void);

protected:
  void changeEvent(QEvent* pEvent);

private:
  void setUIText(void);
  void fillFields(void);

  Ui::UserInfoWindow ui;
  lmcSettings* pSettings;
  XmlMessage userInfo;
};
