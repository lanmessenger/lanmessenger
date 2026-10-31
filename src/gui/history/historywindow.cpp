// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "historywindow.h"
#include "historytreewidget.h"
#include <QGuiApplication>
#include <QLocale>
#include <QScreen>
#include <QTreeWidgetItem>
#include <QKeyEvent>
#include "core/lmccategories.h"
#include "core/definitions.h"
#include "core/settings.h"
#include "gui/chat/chatmessageview.h"
#include "messaging/xmlmessage.h"
#include "storage/historystore.h"

lmcHistoryWindow::lmcHistoryWindow(QWidget* parent, Qt::WindowFlags flags)
    : QWidget(parent, flags) {
  ui.setupUi(this);

  //	Destroy the window when it closes
  setAttribute(Qt::WA_DeleteOnClose, true);

  pMessageLog = new ChatMessageView(ui.fraMessageLog);
  ui.logLayout->addWidget(pMessageLog);

  QList<int> sizes;
  sizes.append(width() * 0.35);
  sizes.append(width() - width() * 0.35 - ui.splitter->handleWidth());
  ui.splitter->setSizes(sizes);
  QRect scr = QGuiApplication::primaryScreen()->geometry();
  move(scr.center() - rect().center());

  connect(ui.tvMsgList, &QTreeWidget::currentItemChanged,
          this, &lmcHistoryWindow::tvMsgList_currentItemChanged);
  connect(ui.btnClearHistory, &QAbstractButton::clicked, this, &lmcHistoryWindow::btnClearHistory_clicked);
  connect(HistoryStore::instance(), &HistoryStore::sessionsChanged, this,
          &lmcHistoryWindow::updateList);

  ui.tvMsgList->installEventFilter(this);
  pMessageLog->installEventFilter(this);
  ui.btnClearHistory->installEventFilter(this);
  ui.btnClose->installEventFilter(this);
}

lmcHistoryWindow::~lmcHistoryWindow() {
  qCDebug(lmcUi(), "History window destroyed");
}

void lmcHistoryWindow::init(void) {
  setWindowIcon(QIcon(IDR_APPICON));
  ui.splitter->setStyleSheet("QSplitter::handle { image: url(" IDR_HGRIP "); }");

  pMessageLog->setAutoScroll(false);

  pSettings = new lmcSettings();
  //  Single chat view shares the saved theme and appearance mode.
  pMessageLog->setThemeName(pSettings->value(IDS_THEME, IDS_THEME_VAL).toString());
  pMessageLog->setAppearanceMode(
    pSettings->value(IDS_APPEARANCE_MODE, IDS_APPEARANCE_MODE_VAL).toString());
  //  Structured history keys own rows on this sentinel (right side).
  pMessageLog->localId = ChatMessageView::historyLocalId();
  pMessageLog->initChat();
  restoreGeometry(pSettings->value(IDS_WINDOWHISTORY).toByteArray());
  ui.splitter->restoreState(pSettings->value(IDS_SPLITTERHISTORY).toByteArray());
  setUIText();

  displayList();
}

void lmcHistoryWindow::updateList(void) {
  displayList();
}

void lmcHistoryWindow::stop(void) {
  pSettings->setValue(IDS_WINDOWHISTORY, saveGeometry());
  pSettings->setValue(IDS_SPLITTERHISTORY, ui.splitter->saveState());
}

void lmcHistoryWindow::settingsChanged(void) {
  //  Legacy window ignored settings; theme and appearance mode are live
  //  (flags keep view defaults, as before).
  QString themeName = pSettings->value(IDS_THEME, IDS_THEME_VAL).toString();
  if(themeName.compare(pMessageLog->themeNameString()) != 0)
    pMessageLog->setThemeName(themeName);
  QString appearanceMode =
    pSettings->value(IDS_APPEARANCE_MODE, IDS_APPEARANCE_MODE_VAL).toString();
  if(appearanceMode.compare(pMessageLog->appearanceModeString()) != 0)
    pMessageLog->setAppearanceMode(appearanceMode);
}

bool lmcHistoryWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
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

void lmcHistoryWindow::changeEvent(QEvent* pEvent) {
  switch(pEvent->type()) {
  case QEvent::LanguageChange:
    setUIText();
    break;
  default:
    break;
  }

  QWidget::changeEvent(pEvent);
}

void lmcHistoryWindow::tvMsgList_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
  Q_UNUSED(previous);

  if(current) {
    qint64 sessionId = current->data(0, DataRole).toLongLong();
    QList<StoredMessage> rows = HistoryStore::instance()->messagesForSession(sessionId);
    QDateTime closedAt;
    for(const SessionInfo& s : msgList) {
      if(s.id == sessionId) {
        closedAt = s.closedAt;
        break;
      }
    }
    pMessageLog->initChat();
    for(const StoredMessage& row : rows) {
      XmlMessage xml;
      qint64 ts = row.createdAt > 0 ? row.createdAt
                                    : (closedAt.isValid() ? closedAt.toMSecsSinceEpoch() : 0);
      xml.addHeader(XN_TIME, QString::number(ts));
      xml.addData(XN_MESSAGE, row.body);
      QString userId = row.isOwn ? pMessageLog->localId
                                 : (!row.senderId.isEmpty() ? row.senderId : row.senderName);
      QString name = row.senderName;
      MessageType type =
        row.messageType == int(MT_GroupMessage) ? MT_GroupMessage : MT_Message;
      pMessageLog->appendMessageLog(type, &userId, &name, &xml);
    }
  }
}

void lmcHistoryWindow::btnClearHistory_clicked(void) {
  // Success arrives via sessionsChanged; no explicit displayList() (masks store failure).
  if(!HistoryStore::instance()->clearSessions())
    qCWarning(lmcUi(), "History window: clearSessions failed");
}

void lmcHistoryWindow::setUIText(void) {
  ui.retranslateUi(this);

  setWindowTitle(tr("Message History"));
}

void lmcHistoryWindow::displayList(void) {
  qint64 selectedId = 0;
  if(ui.tvMsgList->currentItem())
    selectedId = ui.tvMsgList->currentItem()->data(0, DataRole).toLongLong();
  pMessageLog->initChat();
  ui.tvMsgList->clear();
  msgList.clear();

  msgList = HistoryStore::instance()->listSessions();

  QTreeWidgetItem* toSelect = nullptr;
  for(int index = 0; index < msgList.count(); index++) {
    lmcHistoryTreeWidgetItem* pItem = new lmcHistoryTreeWidgetItem();
    pItem->setText(0, msgList[index].name);
    pItem->setText(1, QLocale::system().toString(msgList[index].closedAt, QLocale::ShortFormat));
    pItem->setData(0, DataRole, msgList[index].id);
    pItem->setData(1, DataRole, msgList[index].closedAt);
    pItem->setSizeHint(0, QSize(0, 20));
    ui.tvMsgList->addTopLevelItem(pItem);
    if(msgList[index].id == selectedId)
      toSelect = pItem;
  }

  ui.tvMsgList->sortByColumn(1, Qt::DescendingOrder);

  if(toSelect)
    ui.tvMsgList->setCurrentItem(toSelect);
  else if(ui.tvMsgList->topLevelItemCount() > 0)
    ui.tvMsgList->setCurrentItem(ui.tvMsgList->topLevelItem(0));
}
