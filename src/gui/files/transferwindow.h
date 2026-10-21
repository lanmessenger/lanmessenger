// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QDir>
#include <QToolBar>
#include <QToolButton>
#include <qevent.h>
#include "ui_transferwindow.h"
#include "core/shared.h"
#include "core/settings.h"
#include "core/stdlocation.h"
#include "core/soundplayer.h"
#include "messaging/xmlmessage.h"

class lmcTransferWindow : public QWidget {
  Q_OBJECT

public:
  lmcTransferWindow(QWidget* parent = 0);
  ~lmcTransferWindow(void);

  void init(void);
  void updateList(void);
  void stop(void);
  void createTransfer(MessageType type, FileMode mode, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage);
  void receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void settingsChanged(void);

signals:
  void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void showTrayMessage(TrayMessageType type, QString szMessage, QString szTitle, TrayMessageIcon icon);

protected:
  bool eventFilter(QObject* pObject, QEvent* pEvent);
  void changeEvent(QEvent* pEvent);

private slots:
  void lvTransferList_currentRowChanged(int currentRow);
  void lvTransferList_activated(const QModelIndex& index);
  void btnCancel_clicked(void);
  void btnRemove_clicked(void);
  void btnClear_clicked(void);
  void btnShowFolder_clicked(void);
  void updateProgress(FileView* view, qint64 currentPos);

private:
  void createToolBar(void);
  void setUIText(void);
  void setButtonState(FileView::TransferState state);
  QPixmap getIcon(QString filePath);
  QString formatTime(qint64 size, qint64 speed);
  void clearList(void);

  Ui::TransferWindow ui;
  lmcSettings* pSettings;
  lmcSoundPlayer* pSoundPlayer;
  QAction* pactCancel;
  QAction* pactShowFolder;
  QAction* pactRemove;
  QList<FileView> pendingSendList;
};
