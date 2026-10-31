// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "transferwindow.h"
#include <QDesktopServices>
#include <QDateTime>
#include <QGuiApplication>
#include <QHash>
#include <QScreen>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QToolBar>
#include <QFile>
#include <QKeyEvent>
#include <QSignalBlocker>
#include <algorithm>
#include "core/shared.h"
#include "core/lmccategories.h"
#include "core/settings.h"
#include "core/stdlocation.h"
#include "core/soundplayer.h"
#include "messaging/xmlmessage.h"
#include "gui/widgets/actionhelper.h"
#include "storage/historystore.h"

lmcTransferWindow::lmcTransferWindow(QWidget* parent)
    : QWidget(parent) {
  ui.setupUi(this);
  QRect scr = QGuiApplication::primaryScreen()->geometry();
  move(scr.center() - rect().center());

  connect(ui.lvTransferList, &lmcTransferListView::currentRowChanged, this, &lmcTransferWindow::lvTransferList_currentRowChanged);
  connect(ui.lvTransferList, &QAbstractItemView::activated,
          this, &lmcTransferWindow::lvTransferList_activated);
  connect(ui.btnClear, &QAbstractButton::clicked, this, &lmcTransferWindow::btnClear_clicked);

  ui.lvTransferList->installEventFilter(this);
  ui.btnClear->installEventFilter(this);
  ui.btnClose->installEventFilter(this);

  pendingSendList.clear();
}

lmcTransferWindow::~lmcTransferWindow(void) {
}

void lmcTransferWindow::init(void) {
  setWindowIcon(QIcon(IDR_APPICON));

  createToolBar();
  setButtonState(FileView::TS_Max);

  pSettings = new lmcSettings();
  restoreGeometry(pSettings->value(IDS_WINDOWTRANSFERS).toByteArray());
  setUIText();

  loadFromStore();
  if(ui.lvTransferList->count() > 0)
    ui.lvTransferList->setCurrentRow(0);

  pSoundPlayer = new lmcSoundPlayer();
  connect(HistoryStore::instance(), &HistoryStore::transfersChanged, this,
          &lmcTransferWindow::updateList);
}

void lmcTransferWindow::updateList(void) {
  // External change: reload finished rows from the store, keep rows the
  // store never had (active + unsaved terminal), prefer the live copy for
  // active ones. Rows already saved before (in storeKeys) are trusted to
  // the store, so an external clear/remove does not resurrect in the view.
  // With a closed store there is nothing authoritative to merge against:
  // keep the live view instead of wiping finished rows.
  if(!HistoryStore::instance()->isOpen())
    return;
  QList<FileView> snapshot;
  for(int i = 0; i < ui.lvTransferList->count(); i++) {
    FileView* v = ui.lvTransferList->item(i);
    if(v)
      snapshot.append(*v);
  }
  QSet<QString> oldKeys = storeKeys;
  QList<FileView> storeViews = viewsFromStore();
  QList<FileView> merged = mergeTransferViews(storeViews, snapshot, oldKeys);
  while(ui.lvTransferList->count() > 0)
    ui.lvTransferList->removeItem(0);
  for(int i = 0; i < merged.count(); i++)
    ui.lvTransferList->insertItem(i, &merged[i]);
  if(ui.lvTransferList->count() > 0 && ui.lvTransferList->currentRow() < 0)
    ui.lvTransferList->setCurrentRow(0);
}

QList<FileView> lmcTransferWindow::mergeTransferViews(const QList<FileView>& storeViews,
                                                      const QList<FileView>& snapshot,
                                                      const QSet<QString>& oldKeys) {
  QSet<QString> storeKeySet;
  for(const FileView& v : storeViews)
    storeKeySet.insert(transferKey(v.id, int(v.mode)));
  // Live (active) snapshot rows win over stale store rows with the same key.
  QHash<QString, FileView> live;
  // Finished rows the store never had (unsaved terminal) are kept.
  QList<FileView> unsaved;
  for(const FileView& row : snapshot) {
    const QString key = transferKey(row.id, int(row.mode));
    if(row.state < FileView::TS_Complete)
      live.insert(key, row);
    else if(!oldKeys.contains(key) && !storeKeySet.contains(key))
      unsaved.append(row);
  }
  QList<FileView> merged;
  QSet<QString> consumedLive;
  for(const FileView& sv : storeViews) {
    const QString key = transferKey(sv.id, int(sv.mode));
    if(live.contains(key)) {
      merged.append(live.value(key));
      consumedLive.insert(key);
    } else {
      merged.append(sv);
    }
  }
  // Active snapshot rows the store never had (new transfers) have no store
  // row to replace and are appended here; the stable sort below slots them
  // into chronological position.
  for(auto it = live.constBegin(); it != live.constEnd(); ++it) {
    if(!consumedLive.contains(it.key()))
      merged.append(it.value());
  }
  merged.append(unsaved);
  // Chronological view: the store arrives ORDER BY created_at ASC, so a
  // stable sort by startTime slots the surviving snapshot rows into
  // their own time position instead of pinning them on top.
  // Tie-break by id: QHash iteration is random, two transfers in the same
  // millisecond must merge deterministically.
  std::stable_sort(merged.begin(), merged.end(), [](const FileView& a, const FileView& b) {
    return a.startTime != b.startTime ? a.startTime < b.startTime : a.id < b.id;
  });
  return merged;
}

QString lmcTransferWindow::transferKey(const QString& id, int mode) {
  return id + QLatin1Char('\x1f') + QString::number(mode);
}

void lmcTransferWindow::stop(void) {
  //	Cancel all active transfers
  for(int index = 0; index < ui.lvTransferList->count(); index++) {
    FileView* view = ui.lvTransferList->item(index);

    if(view->state < FileView::TS_Complete) {
      int mode = view->mode == FileView::TM_Send ? FM_Send : FM_Receive;
      XmlMessage xmlMessage;
      xmlMessage.addData(XN_MODE, FileModeNames[mode]);
      xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
      xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Cancel]);
      xmlMessage.addData(XN_FILEID, view->id);
      emit messageSent((MessageType)view->type, &view->userId, &xmlMessage);

      view->state = FileView::TS_Cancel;
    }
  }

  bool saveHistory = pSettings->value(IDS_FILEHISTORY, IDS_FILEHISTORY_VAL).toBool();
  if(saveHistory)
    saveToStore();
  pSettings->setValue(IDS_WINDOWTRANSFERS, saveGeometry());
}

void lmcTransferWindow::createTransfer(MessageType type, FileMode mode, QString* lpszUserId, QString* lpszUserName, XmlMessage* pMessage) {
  FileView fileView(pMessage->data(XN_FILEID));
  fileView.fileSize = pMessage->data(XN_FILESIZE).toLongLong();
  fileView.sizeDisplay = Helper::formatSize(fileView.fileSize);
  fileView.userId = *lpszUserId;
  fileView.userName = *lpszUserName;
  fileView.fileName = pMessage->data(XN_FILENAME);
  fileView.filePath = pMessage->data(XN_FILEPATH);
  fileView.type = type;
  if(mode == FM_Send) {
    fileView.mode = FileView::TM_Send;
    fileView.state = FileView::TS_Send;
  } else {
    fileView.mode = FileView::TM_Receive;
    fileView.state = FileView::TS_Receive;
  }
  fileView.fileDisplay = fileView.fileName + " (" + fileView.sizeDisplay + ")";
  fileView.icon = getIcon(fileView.filePath);
  fileView.startTime = QDateTime::currentDateTime();
  ui.lvTransferList->insertItem(0, &fileView);
  ui.lvTransferList->setCurrentRow(0);
}

void lmcTransferWindow::receiveMessage(MessageType type, QString* lpszUserId, XmlMessage* pMessage) {
  Q_UNUSED(type);
  Q_UNUSED(lpszUserId);

  int fileMode = Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));
  int fileOp = Helper::indexOf(FileOpNames, FO_Max, pMessage->data(XN_FILEOP));
  QString id = pMessage->data(XN_FILEID);

  FileView* view = nullptr;
  FileView::TransferMode transferMode = fileMode == FM_Send ? FileView::TM_Send : FileView::TM_Receive;
  int itemIndex = -1;
  QString trayMsg;

  switch(fileOp) {
  case FO_Decline:
    //	receiver has declined
    view = ui.lvTransferList->item(id, FileView::TM_Send);
    if(!view)
      return;
    itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Send);
    view->state = FileView::TS_Decline;
    break;
  case FO_Cancel:
    view = ui.lvTransferList->item(id, transferMode);
    if(!view)
      return;
    itemIndex = ui.lvTransferList->itemIndex(id, transferMode);
    view->state = FileView::TS_Cancel;
    break;
  case FO_Progress:
    view = ui.lvTransferList->item(id, transferMode);
    if(!view)
      return;
    itemIndex = ui.lvTransferList->itemIndex(id, transferMode);
    updateProgress(view, pMessage->data(XN_FILESIZE).toLongLong());
    break;
  case FO_Error:
    view = ui.lvTransferList->item(id, transferMode);
    if(!view)
      return;
    itemIndex = ui.lvTransferList->itemIndex(id, transferMode);
    view->state = FileView::TS_Abort;
    break;
  case FO_Abort:
    view = ui.lvTransferList->item(id, transferMode);
    if(!view)
      return;
    itemIndex = ui.lvTransferList->itemIndex(id, transferMode);
    view->state = FileView::TS_Abort;
    break;
  case FO_Complete:
    if(fileMode == FM_Send) {
      view = ui.lvTransferList->item(id, FileView::TM_Send);
      if(!view)
        return;
      itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Send);
      view->state = FileView::TS_Complete;
      if(isHidden() || !isActiveWindow()) {
        trayMsg = tr("'%1' has been sent to %2.");
        emit showTrayMessage(TM_Transfer, trayMsg.arg(view->fileName, view->userName),
                             tr("File Transfer Completed"), TMI_Info);
        pSoundPlayer->play(SE_FileDone);
      }
    } else {
      view = ui.lvTransferList->item(id, FileView::TM_Receive);
      if(!view)
        return;
      itemIndex = ui.lvTransferList->itemIndex(id, FileView::TM_Receive);
      view->filePath = QDir::fromNativeSeparators(pMessage->data(XN_FILEPATH));
      view->icon = getIcon(view->filePath);
      pactShowFolder->setEnabled(QFile::exists(view->filePath));
      view->state = FileView::TS_Complete;
      if(isHidden() || !isActiveWindow()) {
        trayMsg = tr("'%1' has been received from %2.");
        emit showTrayMessage(TM_Transfer, trayMsg.arg(view->fileName, view->userName),
                             tr("File Transfer Completed"), TMI_Info);
        pSoundPlayer->play(SE_FileDone);
      }
    }
    break;
  }

  ui.lvTransferList->itemChanged(itemIndex);

  FileView* current = ui.lvTransferList->currentItem();
  setButtonState(current->state);
}

void lmcTransferWindow::settingsChanged(void) {
  pSoundPlayer->settingsChanged();
}

bool lmcTransferWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
  Q_UNUSED(pObject);
  if(pEvent->type() == QEvent::KeyPress) {
    QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
    if(pKeyEvent->key() == Qt::Key_Escape) {
      close();
      return true;
    }
  }

  return false;
}

void lmcTransferWindow::changeEvent(QEvent* pEvent) {
  switch(pEvent->type()) {
  case QEvent::LanguageChange:
    setUIText();
    break;
  default:
    break;
  }

  QWidget::changeEvent(pEvent);
}

void lmcTransferWindow::lvTransferList_currentRowChanged(int currentRow) {
  if(currentRow < 0) {
    setButtonState(FileView::TS_Max);
    return;
  }

  FileView* pFileView = ui.lvTransferList->item(currentRow);
  setButtonState(pFileView->state);
  pactShowFolder->setEnabled(QFile::exists(pFileView->filePath));
}

void lmcTransferWindow::lvTransferList_activated(const QModelIndex& index) {
  FileView* view = ui.lvTransferList->item(index.row());

  QDesktopServices::openUrl(QUrl::fromLocalFile(view->filePath));
}

void lmcTransferWindow::btnCancel_clicked(void) {
  FileView* view = ui.lvTransferList->currentItem();
  if(!view)
    return;

  int mode = view->mode == FileView::TM_Send ? FM_Send : FM_Receive;
  XmlMessage xmlMessage;
  xmlMessage.addData(XN_MODE, FileModeNames[mode]);
  xmlMessage.addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
  xmlMessage.addData(XN_FILEOP, FileOpNames[FO_Cancel]);
  xmlMessage.addData(XN_FILEID, view->id);
  emit messageSent((MessageType)view->type, &view->userId, &xmlMessage);

  view->state = FileView::TS_Cancel;

  ui.lvTransferList->itemChanged(ui.lvTransferList->currentRow());
  setButtonState(view->state);
}

void lmcTransferWindow::btnRemove_clicked(void) {
  FileView* view = ui.lvTransferList->currentItem();

  if(!view || view->state < FileView::TS_Complete)
    return;

  // Manual view update: block the self-emit to avoid updateList() wiping
  // all finished rows (the signal is for external changes).
  const QSignalBlocker blocker(HistoryStore::instance());
  if(!HistoryStore::instance()->removeTransfer(view->id, int(view->mode))) {
    if(!HistoryStore::instance()->isOpen()) {
      // Closed store: nothing persistent to delete. Drop the live row so the
      // button does not look broken; storeKeys stays untouched so the row
      // correctly resurrects from disk once the driver is back.
      qCWarning(lmcApp(), "Transfers: removeTransfer failed (store closed), "
                          "removing from the view only");
      ui.lvTransferList->removeItem(ui.lvTransferList->currentRow());
      emit showTrayMessage(TM_Transfer,
                           tr("History database is unavailable, the entry was "
                              "removed from the view only."),
                           tr("File Transfers"), TMI_Warning);
    } else {
      emit showTrayMessage(TM_Transfer, tr("Failed to remove the entry from history."),
                           tr("File Transfers"), TMI_Warning);
    }
    return;
  }
  storeKeys.remove(transferKey(view->id, int(view->mode)));
  ui.lvTransferList->removeItem(ui.lvTransferList->currentRow());
}

void lmcTransferWindow::btnClear_clicked(void) {
  const QSignalBlocker blocker(HistoryStore::instance());
  if(!HistoryStore::instance()->clearTransfers()) {
    if(!HistoryStore::instance()->isOpen()) {
      // Same as btnRemove_clicked: keep the live view usable without a
      // driver; storeKeys stays so disk rows resurrect on reopen.
      qCWarning(lmcApp(), "Transfers: clearTransfers failed (store closed), "
                          "clearing the view only");
      clearList();
      emit showTrayMessage(TM_Transfer,
                           tr("History database is unavailable, the list was "
                              "cleared in the view only."),
                           tr("File Transfers"), TMI_Warning);
    } else {
      emit showTrayMessage(TM_Transfer, tr("Failed to clear file history."),
                           tr("File Transfers"), TMI_Warning);
    }
    return;
  }
  storeKeys.clear();
  clearList();
}

void lmcTransferWindow::btnShowFolder_clicked(void) {
  FileView* view = ui.lvTransferList->currentItem();

  QString path = QFileInfo(view->filePath).dir().path();
  QUrl url;
  // hack to make sure qdesktopservices open a network path
  if(path.startsWith("\\\\") || path.startsWith("//"))
    url.setUrl(QDir::toNativeSeparators(path));
  else
    url = QUrl::fromLocalFile(path);

  QDesktopServices::openUrl(url);
}

void lmcTransferWindow::createToolBar(void) {
  QToolBar* pToolBar = new QToolBar(ui.wgtToolBar);
  pToolBar->setIconSize(QSize(16, 16));
  pToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  ui.toolBarLayout->addWidget(pToolBar);

  pactCancel = ActionHelper::createAction(pToolBar, "Cancel", this, &lmcTransferWindow::btnCancel_clicked,
                                          QIcon(QPixmap(IDR_STOP, "PNG")));
  pToolBar->addSeparator();
  pactShowFolder = ActionHelper::createAction(pToolBar, "Show In Folder", this,
                                              &lmcTransferWindow::btnShowFolder_clicked,
                                              QIcon(QPixmap(IDR_FOLDER, "PNG")));
  pactRemove = ActionHelper::createAction(pToolBar, "Remove From List", this, &lmcTransferWindow::btnRemove_clicked,
                                          QIcon(QPixmap(IDR_DECLINE, "PNG")));
}

void lmcTransferWindow::setUIText(void) {
  ui.retranslateUi(this);

  setWindowTitle(tr("File Transfers"));

  pactCancel->setText(tr("Cancel"));
  pactShowFolder->setText(tr("Show In Folder"));
  pactRemove->setText(tr("Remove From List"));
}

void lmcTransferWindow::setButtonState(FileView::TransferState state) {
  switch(state) {
  case FileView::TS_Send:
  case FileView::TS_Receive:
    pactCancel->setEnabled(true);
    pactRemove->setEnabled(false);
    break;
  case FileView::TS_Decline:
  case FileView::TS_Complete:
  case FileView::TS_Cancel:
  case FileView::TS_Abort:
    pactCancel->setEnabled(false);
    pactRemove->setEnabled(true);
    break;
  default:
    pactCancel->setEnabled(false);
    pactShowFolder->setEnabled(false);
    pactRemove->setEnabled(false);
    break;
  }
}

QPixmap lmcTransferWindow::getIcon(QString filePath) {
  QFileIconProvider iconProvider;
  QFileInfo fileInfo(filePath);
  QPixmap icon;

  if(QFile::exists(filePath))
    icon = iconProvider.icon(fileInfo).pixmap(32, 32);
  else {
    QString fileName = fileInfo.fileName();
    QString path = QDir::temp().absoluteFilePath(fileName);
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.close();
    icon = iconProvider.icon(QFileInfo(path)).pixmap(32, 32);
    QFile::remove(path);
  }

  return icon;
}

QString lmcTransferWindow::formatTime(qint64 size, qint64 speed) {
  int d = 86400;
  int h = 3600;
  int m = 60;

  if(speed == 0)
    return tr("Calculating time");

  int time = size / speed;
  QString s;
  if(time > d) {
    s.append(QString("%1d ").arg(time / d));
    time %= d;
  }
  if(time > h) {
    s.append(QString("%1h ").arg(time / h));
    time %= h;
  }
  if(time > m) {
    s.append(QString("%1m ").arg(time / m));
    time %= m;
  }
  if(time >= 0)
    s.append(QString("%1s ").arg(time));

  return s.trimmed();
}

void lmcTransferWindow::clearList(void) {
  for(int index = 0; index < ui.lvTransferList->count(); index++) {
    FileView* view = ui.lvTransferList->item(index);
    if(view->state < FileView::TS_Complete)
      continue;

    ui.lvTransferList->removeItem(index);
    index--;
  }
}

void lmcTransferWindow::loadFromStore(void) {
  // Closed store: nothing authoritative to load; keep storeKeys untouched so
  // the next updateList() merge still knows what the store last contained.
  if(!HistoryStore::instance()->isOpen())
    return;
  QList<FileView> views = viewsFromStore();
  for(int i = 0; i < views.count(); i++)
    ui.lvTransferList->insertItem(ui.lvTransferList->count(), &views[i]);
}

QList<FileView> lmcTransferWindow::viewsFromStore(void) {
  QList<FileView> views;
  QList<TransferRecord> records = HistoryStore::instance()->loadTransfers();
  storeKeys.clear();
  for(const TransferRecord& r : records) {
    FileView view(r.id);
    view.type = (r.messageType == int(MT_Folder)) ? MT_Folder : MT_File;
    if(r.mode == int(FileView::TM_Send) || r.mode == int(FileView::TM_Receive))
      view.mode = (FileView::TransferMode)r.mode;
    else
      view.mode = FileView::TM_Send;
    view.filePath = r.filePath;
    view.fileName = QFileInfo(r.filePath).fileName();
    view.fileDisplay = r.fileDisplay;
    view.userId = r.userId;
    view.userName = r.userName;
    view.fileSize = r.fileSize;
    if(r.state >= 0 && r.state < int(FileView::TS_Max))
      view.state = (FileView::TransferState)r.state;
    else
      view.state = FileView::TS_Complete;
    view.icon = getIcon(view.filePath);
    view.sizeDisplay = Helper::formatSize(view.fileSize);
    if(r.createdAt > 0)
      view.startTime = QDateTime::fromMSecsSinceEpoch(r.createdAt);
    else
      view.startTime = QDateTime::currentDateTime();
    storeKeys.insert(transferKey(view.id, int(view.mode)));
    views.append(view);
  }
  return views;
}

void lmcTransferWindow::saveToStore(void) {
  QList<TransferRecord> records;
  for(int i = 0; i < ui.lvTransferList->count(); i++) {
    FileView* v = ui.lvTransferList->item(i);
    if(!v)
      continue;
    TransferRecord r;
    r.id = v->id;
    r.mode = int(v->mode);
    r.messageType = int(v->type);
    r.filePath = v->filePath;
    r.fileDisplay = v->fileDisplay;
    r.userId = v->userId;
    r.userName = v->userName;
    r.fileSize = v->fileSize;
    r.state = int(v->state);
    r.createdAt = v->startTime.isValid() ? v->startTime.toMSecsSinceEpoch()
                                         : QDateTime::currentMSecsSinceEpoch();
    records.append(r);
  }
  const QSignalBlocker blocker(HistoryStore::instance());
  if(!HistoryStore::instance()->saveTransfers(records))
    return;
  storeKeys.clear();
  for(const TransferRecord& r : records)
    storeKeys.insert(transferKey(r.id, r.mode));
}

void lmcTransferWindow::updateProgress(FileView* view, qint64 currentPos) {
  view->position = currentPos;
  view->posDisplay = Helper::formatSize(view->position);
  qint64 timeSpan = view->startTime.msecsTo(QDateTime::currentDateTime()) / 1000;
  qint64 speed = (timeSpan > 0) ? view->position / timeSpan : 0;
  view->speed = (speed + view->speed) / 2;
  view->speedDisplay = Helper::formatSize(view->speed) + tr("/sec");
  view->timeDisplay = formatTime(view->fileSize - view->position, view->speed);
}
