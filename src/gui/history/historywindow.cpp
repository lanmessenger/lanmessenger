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
#include <QFile>
#include "core/lmccategories.h"
#include "core/settings.h"
#include "gui/chat/chatmessageview.h"

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
    qint64 offset = current->data(0, DataRole).toLongLong();
    QString data = History::getMessage(offset);
    QString name = current->text(0);
    QDateTime date = current->data(1, DataRole).toDateTime();
    //  S1.5: stored blobs (legacy HTML or new-model HTML) render as one
    //  read-only body row; stored history is never migrated.
    pMessageLog->showLegacyHtml(&name, &date, &data);
  }
}

void lmcHistoryWindow::btnClearHistory_clicked(void) {
  QFile::remove(History::historyFile());
  displayList();
}

void lmcHistoryWindow::setUIText(void) {
  ui.retranslateUi(this);

  setWindowTitle(tr("Message History"));
}

void lmcHistoryWindow::displayList(void) {
  pMessageLog->initChat();
  ui.tvMsgList->clear();
  msgList.clear();

  msgList = History::getList();

  for(int index = 0; index < msgList.count(); index++) {
    lmcHistoryTreeWidgetItem* pItem = new lmcHistoryTreeWidgetItem();
    pItem->setText(0, msgList[index].name);
    pItem->setText(1, QLocale::system().toString(msgList[index].date, QLocale::ShortFormat));
    pItem->setData(0, DataRole, msgList[index].offset);
    pItem->setData(1, DataRole, msgList[index].date);
    pItem->setSizeHint(0, QSize(0, 20));
    ui.tvMsgList->addTopLevelItem(pItem);
  }

  ui.tvMsgList->sortByColumn(1, Qt::DescendingOrder);

  if(ui.tvMsgList->topLevelItemCount() > 0)
    ui.tvMsgList->setCurrentItem(ui.tvMsgList->topLevelItem(0));
}
