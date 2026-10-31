// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include <QPixmap>
#include <QSet>
#include "ui_transferwindow.h"
#include "core/definitions.h"
#include "gui/files/filemodelview.h"
#include "storage/historystore.h"

class lmcSettings;
class lmcSoundPlayer;
class XmlMessage;
class QAction;
class QModelIndex;
class QEvent;

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

  // Pure merge policy for updateList(), unit-tested in tst_transfermerge:
  // store rows are authoritative for keys seen before (oldKeys), live
  // active snapshot rows win over stale store rows with the same key,
  // finished rows the store never had are kept; the result is ordered
  // chronologically by startTime (store rows arrive ORDER BY created_at).
  static QList<FileView> mergeTransferViews(const QList<FileView>& storeViews,
                                            const QList<FileView>& snapshot,
                                            const QSet<QString>& oldKeys);

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
  void loadFromStore(void);
  QList<FileView> viewsFromStore(void);
  void saveToStore(void);
  static QString transferKey(const QString& id, int mode);

  Ui::TransferWindow ui;
  lmcSettings* pSettings;
  lmcSoundPlayer* pSoundPlayer;
  QAction* pactCancel;
  QAction* pactShowFolder;
  QAction* pactRemove;
  QList<FileView> pendingSendList;
  // Keys (id + mode) last seen in the store. updateList() trusts the store
  // for these and re-inserts only rows the store never had (unsaved new
  // terminal rows); otherwise an external clear would resurrect in the view.
  QSet<QString> storeKeys;
};
