// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QDialog>
#include "ui_userselectdialog.h"
#include "core/shared.h"
#include "core/settings.h"

class lmcUserSelectDialog : public QDialog {
  Q_OBJECT

public:
  lmcUserSelectDialog(QWidget* parent = 0);
  ~lmcUserSelectDialog();

  void init(QList<QTreeWidgetItem*>* pContactsList);

  QStringList selectedContacts;

protected:
  void changeEvent(QEvent* pEvent);

private slots:
  void btnOK_clicked(void);
  void tvUserList_itemChanged(QTreeWidgetItem* item, int column);

private:
  void setUIText(void);

  Ui::UserSelectDialog ui;
  lmcSettings* pSettings;
  bool parentToggling;
  bool childToggling;
  int selectedCount;
};
