// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "broadcastwindow.h"
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QIcon>
#include <QPixmap>
#include "core/settings.h"
#include "core/uidefinitions.h"
#include "messaging/xmlmessage.h"
#include "gui/chat/chatdefinitions.h"
#include "gui/widgets/subcontrols.h"
#include "gui/widgets/infobar.h"
#include "gui/chat/emojipicker.h"
#include "gui/chat/emojipickeraction.h"
#include "gui/chat/emojihighlighter.h"
#include "gui/chat/chathelper.h"

namespace {

//	Absolute composer scale, deliberately distinct from kThemePointSize.
constexpr qreal kComposerSmallPt = 8.25;
constexpr qreal kComposerMediumPt = 11.0;
constexpr qreal kComposerLargePt = 14.0;

} // namespace

//	constructor
lmcBroadcastWindow::lmcBroadcastWindow(QWidget* parent)
    : QWidget(parent) {
  ui.setupUi(this);

  //	Destroy the window when it closes
  setAttribute(Qt::WA_DeleteOnClose, true);

  //	Composer takes ~60%, user list the rest; style/DPI agnostic.
  ui.splitter->setStretchFactor(0, 3);
  ui.splitter->setStretchFactor(1, 2);

  connect(ui.btnSelectAll, &QAbstractButton::clicked, this, &lmcBroadcastWindow::btnSelectAll_clicked);
  connect(ui.btnSelectNone, &QAbstractButton::clicked, this, &lmcBroadcastWindow::btnSelectNone_clicked);
  connect(ui.tvUserList, &QTreeWidget::itemChanged,
          this, &lmcBroadcastWindow::tvUserList_itemChanged);
  connect(ui.btnSend, &QAbstractButton::clicked, this, &lmcBroadcastWindow::btnSend_clicked);

  //	event filters for handling keyboard input
  ui.txtMessage->installEventFilter(this);
  ui.tvUserList->installEventFilter(this);
  ui.btnSend->installEventFilter(this);
  ui.btnCancel->installEventFilter(this);
  ui.btnSelectAll->installEventFilter(this);
  ui.btnSelectNone->installEventFilter(this);

  infoFlag = IT_Ok;

  parentToggling = false;
  childToggling = false;
}

lmcBroadcastWindow::~lmcBroadcastWindow() {
}

void lmcBroadcastWindow::init(bool connected) {
  createToolBar();

  setWindowIcon(QIcon(IDR_APPICON));

  ui.tvUserList->setIconSize(QSize(16, 16));
  ui.tvUserList->header()->setSectionsMovable(false);
  ui.tvUserList->header()->setStretchLastSection(false);
  ui.tvUserList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  ui.tvUserList->setCheckable(true);

  //	load settings
  pSettings = new lmcSettings();
  restoreGeometry(pSettings->value(IDS_WINDOWBROADCAST).toByteArray());
  ui.splitter->restoreState(pSettings->value(IDS_SPLITTERBROADCAST).toByteArray());
  //	Forget pre-native handleWidth from saved state; follow the style.
  ui.splitter->setHandleWidth(-1);
  sendKeyMod = pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool();
  composerSizeVal = BFS_Default;
  pFontGroup->actions()[composerSizeVal]->setChecked(true);
  int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
  ui.tvUserList->setView((UserListView)viewType);

  //	show a message if not connected
  bConnected = connected;
  ui.btnSend->setEnabled(bConnected);
  if(!bConnected)
    showStatus(IT_Disconnected, true);

  setUIText();

  applyFontSize();
  //  Composer stays base-only; emoji runs are painted by the highlighter.
  emojiHighlighter = new EmojiHighlighter(ui.txtMessage->document());
  emojiHighlighter->setBaseFamily(ui.txtMessage->font().family());
  ui.txtMessage->setFocus();
}

void lmcBroadcastWindow::stop(void) {
  //	save window geometry and splitter panel sizes
  pSettings->setValue(IDS_WINDOWBROADCAST, saveGeometry());
  pSettings->setValue(IDS_SPLITTERBROADCAST, ui.splitter->saveState());
}

void lmcBroadcastWindow::show(QList<QTreeWidgetItem*>* pGroupList) {
  QWidget::show();

  if(!pGroupList)
    return;

  //	populate the user tree
  ui.tvUserList->clear();
  for(int index = 0; index < pGroupList->count(); index++) {
    QTreeWidgetItem* pItem = pGroupList->value(index);
    pItem->setCheckState(0, Qt::Unchecked);
    for(int childIndex = 0; childIndex < pItem->childCount(); childIndex++)
      pItem->child(childIndex)->setCheckState(0, Qt::Unchecked);
    ui.tvUserList->addTopLevelItem(pItem);
  }
  ui.tvUserList->expandAll();

  btnSelectAll_clicked();
}

void lmcBroadcastWindow::connectionStateChanged(bool connected) {
  bConnected = connected;
  ui.btnSend->setEnabled(bConnected);
  bConnected ? showStatus(IT_Disconnected, false) : showStatus(IT_Disconnected, true);
}

void lmcBroadcastWindow::settingsChanged(void) {
  sendKeyMod = pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool();
  int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
  ui.tvUserList->setView((UserListView)viewType);
  if(composerSizeVal == BFS_Default)
    applyFontSize();
}

//	this method receives keyboard events and check if Enter key or Escape key were pressed
//	if so, corresponding functions are called
bool lmcBroadcastWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
  if(pEvent->type() == QEvent::KeyPress) {
    QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
    if(pKeyEvent->key() == Qt::Key_Escape) {
      close();
      return true;
    } else if((pKeyEvent->key() == Qt::Key_Return || pKeyEvent->key() == Qt::Key_Enter) && pObject == ui.txtMessage) {
      bool keyMod = ((pKeyEvent->modifiers() & Qt::ControlModifier) == Qt::ControlModifier);
      if(keyMod == sendKeyMod) {
        sendMessage();
        return true;
      }
      // The TextEdit widget does not insert new line when Ctrl+Enter is pressed
      // So we insert a new line manually
      if(keyMod)
        ui.txtMessage->insertPlainText("\n");
    }
  }

  return false;
}

void lmcBroadcastWindow::changeEvent(QEvent* pEvent) {
  switch(pEvent->type()) {
  case QEvent::LanguageChange:
    setUIText();
    break;
  default:
    break;
  }

  QWidget::changeEvent(pEvent);
}

void lmcBroadcastWindow::closeEvent(QCloseEvent* pEvent) {
  ui.txtMessage->clear();
  btnSelectNone_clicked();

  QWidget::closeEvent(pEvent);
}

//	change the font size of the text box with toolbar button
void lmcBroadcastWindow::btnFontSize_clicked(void) {
  composerSizeVal = (composerSizeVal == BFS_COUNT - 1) ? 0 : composerSizeVal + 1;
  pFontGroup->actions()[composerSizeVal]->setChecked(true);
  pbtnFontSize->setText(fontSizeName(composerSizeVal));
  applyFontSize();
}

//	change the font size of text box through menu
void lmcBroadcastWindow::fontAction_triggered(QAction* action) {
  composerSizeVal = qBound(0, action->data().toInt(), int(BFS_COUNT) - 1);
  pbtnFontSize->setText(fontSizeName(composerSizeVal));
  applyFontSize();
}

void lmcBroadcastWindow::applyFontSize(void) {
  const qreal size = composerPointSize();
  QFont font = ui.txtMessage->font();
  font.setPointSizeF(size);
  ui.txtMessage->setFont(font);
  QTextCharFormat format;
  format.setFontPointSize(size);
  ChatHelper::mergeFormatOverDocument(ui.txtMessage, format);
}

QString lmcBroadcastWindow::fontSizeName(int size) const {
  switch(size) {
  case BFS_Small:
    return lmcStrings::fontSize()[FS_SMALL];
  case BFS_Medium:
    return lmcStrings::fontSize()[FS_MEDIUM];
  case BFS_Large:
    return lmcStrings::fontSize()[FS_LARGE];
  default:
    return tr("Default size");
  }
}

qreal lmcBroadcastWindow::composerPointSize(void) const {
  //  Default follows the Messages/FontSize setting; explicit sizes travel as XN_FONTSIZE.
  switch(composerSizeVal) {
  case BFS_Small:
    return kComposerSmallPt;
  case BFS_Medium:
    return kComposerMediumPt;
  case BFS_Large:
    return kComposerLargePt;
  default: {
    const int settingsSize = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
    return kThemePointSize[qBound(0, settingsSize, FS_COUNT - 1)];
  }
  }
}

//	insert an emoji into the text box
void lmcBroadcastWindow::emojiAction_triggered(void) {
  if(nEmoji < 0)
    return;
  const QString emoji = EmojiPicker::emojiAt(nEmoji);
  nEmoji = -1;
  if(emoji.isEmpty())
    return;
  ui.txtMessage->insertPlainText(emoji);
}

//	select all users in the user tree
void lmcBroadcastWindow::btnSelectAll_clicked(void) {
  for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++)
    ui.tvUserList->topLevelItem(index)->setCheckState(0, Qt::Checked);
}

//	deselect all users in the user tree
void lmcBroadcastWindow::btnSelectNone_clicked(void) {
  for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++)
    ui.tvUserList->topLevelItem(index)->setCheckState(0, Qt::Unchecked);
}

//	event called when the user checks/unchecks a tree item
void lmcBroadcastWindow::tvUserList_itemChanged(QTreeWidgetItem* item, int column) {
  Q_UNUSED(column);

  //	if parent tree item was toggled, update all its children to the same state
  //	if a child tree item was toggled, two cases arise:
  //		if all its siblings and it are checked, update its parent to checked
  //		if all its siblings and it are not checked, update its parent to unchecked
  if(item->data(0, TypeRole).toString().compare("Group") == 0 && !childToggling) {
    parentToggling = true;
    for(int index = 0; index < item->childCount(); index++)
      item->child(index)->setCheckState(0, item->checkState(0));
    parentToggling = false;
  } else if(item->data(0, TypeRole).toString().compare("User") == 0 && !parentToggling) {
    childToggling = true;
    int nChecked = 0;
    QTreeWidgetItem* parent = item->parent();
    for(int index = 0; index < parent->childCount(); index++)
      if(parent->child(index)->checkState(0))
        nChecked++;
    Qt::CheckState check = (nChecked == parent->childCount()) ? Qt::Checked : Qt::Unchecked;
    parent->setCheckState(0, check);
    childToggling = false;
  }
}

void lmcBroadcastWindow::btnSend_clicked(void) {
  sendMessage();
}

//	create toolbar and add buttons
void lmcBroadcastWindow::createToolBar(void) {
  //	create the toolbar
  pToolBar = new QToolBar(ui.toolBarWidget);
  pToolBar->setIconSize(QSize(16, 16));
  ui.toolBarLayout->addWidget(pToolBar);

  //	create the font menu
  QMenu* pFontMenu = new QMenu(this);
  pFontGroup = new QActionGroup(this);
  connect(pFontGroup, &QActionGroup::triggered, this, &lmcBroadcastWindow::fontAction_triggered);

  for(int index = 0; index < BFS_COUNT; index++) {
    QAction* pAction = new QAction(fontSizeName(index), this);
    pAction->setCheckable(true);
    pAction->setData(index);
    pFontGroup->addAction(pAction);
    pFontMenu->addAction(pAction);
  }

  //	create the font tool button
  pbtnFontSize = new QToolButton(pToolBar);
  pbtnFontSize->setToolButtonStyle(Qt::ToolButtonTextOnly);
  pbtnFontSize->setPopupMode(QToolButton::MenuButtonPopup);
  pbtnFontSize->setMenu(pFontMenu);
  connect(pbtnFontSize, &QAbstractButton::clicked, this, &lmcBroadcastWindow::btnFontSize_clicked);
  pToolBar->addWidget(pbtnFontSize);

  //	create the emoji menu
  pEmojiAction = new EmojiPickerAction(this, &nEmoji);
  connect(pEmojiAction, &QAction::triggered, this, &lmcBroadcastWindow::emojiAction_triggered);

  pEmojiMenu = new QMenu(this);
  pEmojiMenu->addAction(pEmojiAction);

  //	create the emoji tool button
  pbtnEmoji = new lmcToolButton(pToolBar);
  pbtnEmoji->setObjectName(QStringLiteral("btnEmoji"));
  pbtnEmoji->setIcon(QIcon(QPixmap(IDR_SMILEY, "PNG")));
  pbtnEmoji->setPopupMode(QToolButton::InstantPopup);
  pbtnEmoji->setMenu(pEmojiMenu);
  pToolBar->addWidget(pbtnEmoji);
}

void lmcBroadcastWindow::setUIText(void) {
  ui.retranslateUi(this);

  setWindowTitle(tr("Send Broadcast Message"));

  pbtnFontSize->setText(fontSizeName(composerSizeVal));
  pbtnFontSize->setToolTip(tr("Change Font Size"));
  pbtnEmoji->setToolTip(tr("Insert Emoji"));

  for(int index = 0; index < pFontGroup->actions().count(); index++)
    pFontGroup->actions()[index]->setText(fontSizeName(index));
}

//	send the broadcast message to all selected users
void lmcBroadcastWindow::sendMessage(void) {
  //	return if text box is empty
  if(ui.txtMessage->document()->isEmpty())
    return;

  //	send only if connected
  if(bConnected) {
    QString szMessage(ui.txtMessage->toPlainText());

    //	send broadcast
    int sendCount = 0;
    XmlMessage xmlMessage;
    xmlMessage.addData(XN_BROADCAST, szMessage);
    //  Default follows the recipient theme; explicit sizes travel as XN_FONTSIZE.
    if(composerSizeVal != BFS_Default)
      xmlMessage.addData(XN_FONTSIZE, QString::number(composerPointSize(), 'f', 2));
    for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++) {
      for(int childIndex = 0; childIndex < ui.tvUserList->topLevelItem(index)->childCount(); childIndex++) {
        QTreeWidgetItem* item = ui.tvUserList->topLevelItem(index)->child(childIndex);
        if(item->checkState(0) == Qt::Checked) {
          QString szUserId = item->data(0, IdRole).toString();
          emit messageSent(MT_Broadcast, &szUserId, &xmlMessage);
          sendCount++;
        }
      }
    }

    if(sendCount == 0) {
      QMessageBox::warning(this, tr("No recipient selected"),
                           tr("Please select at least one recipient to send a broadcast."));
      return;
    }

    ui.txtMessage->clear();
    close();
  }
}

//	show a message depending on the connection state
void lmcBroadcastWindow::showStatus(int flag, bool add) {
  infoFlag = add ? infoFlag | flag : infoFlag & ~flag;

  if(infoFlag & IT_Disconnected) {
    ui.lblInfo->showMessage(lmcInfoBar::Error,
                            tr("You are no longer connected. Broadcast message cannot be sent."));
  } else {
    ui.lblInfo->clear();
  }
}
