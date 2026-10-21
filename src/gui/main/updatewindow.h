// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef UPDATEWINDOW_H
#define UPDATEWINDOW_H

#include <QWidget>
#include <qevent.h>
#include "messaging/xmlmessage.h"

namespace Ui {
class UpdateWindow;
}

class lmcUpdateWindow : public QWidget {
  Q_OBJECT

public:
  explicit lmcUpdateWindow(QRect* pRect, QWidget* parent = 0);
  ~lmcUpdateWindow();

  void init(void);
  void stop(void);
  void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void settingsChanged(void);

signals:
  void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);

protected:
  bool eventFilter(QObject* pObject, QEvent* pEvent);
  void changeEvent(QEvent* pEvent);

private slots:
  void btnRecheck_clicked(void);

private:
  void setUIText(void);
  void checkForUpdates(void);
  QString getStatusMessage(void);

  enum UpdateStatus {
    US_Check,
    US_Error,
    US_New,
    US_Latest
  };

  Ui::UpdateWindow* ui;
  QString webVersion;
  UpdateStatus status;
};

#endif // UPDATEWINDOW_H
