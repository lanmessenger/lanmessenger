// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include <QMenu>
#include <QHeaderView>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QShowEvent>
#include <QStyleOptionMenuItem>
#include <qmath.h>
#include "emojipicker.h"
#include "gui/chat/chathelper.h"

namespace {

const int kColumns = 8;
const int kCellSize = 32;
const int kFontSizePt = 16;

//  No flags, no ZWJ chains, no skin-tone variants.
//  BMP symbols carry VS16: without it they default to text presentation
//  and the matcher leaves them in the base font.
const QStringList kEmojis = {
  QStringLiteral("\U0001F600"),
  QStringLiteral("\U0001F601"),
  QStringLiteral("\U0001F602"),
  QStringLiteral("\U0001F642"),
  QStringLiteral("\U0001F609"),
  QStringLiteral("\U0001F60A"),
  QStringLiteral("\U0001F60D"),
  QStringLiteral("\U0001F618"),
  QStringLiteral("\U0001F60E"),
  QStringLiteral("\U0001F914"),
  QStringLiteral("\U0001F610"),
  QStringLiteral("\U0001F644"),
  QStringLiteral("\U0001F634"),
  QStringLiteral("\U0001F637"),
  QStringLiteral("\U0001F44D"),
  QStringLiteral("\U0001F44E"),
  QStringLiteral("\U0001F44F"),
  QStringLiteral("\U0001F64F"),
  QStringLiteral("\U0001F4AA"),
  QStringLiteral("\u2764\uFE0F"),
  QStringLiteral("\U0001F494"),
  QStringLiteral("\U0001F525"),
  QStringLiteral("\U0001F389"),
  QStringLiteral("\u2705\uFE0F"),
  QStringLiteral("\u274C\uFE0F"),
  QStringLiteral("\u2753\uFE0F"),
  QStringLiteral("\u2757\uFE0F"),
  QStringLiteral("\U0001F4AF"),
  QStringLiteral("\U0001F440"),
  QStringLiteral("\U0001F44C"),
  QStringLiteral("\U0001F622"),
  QStringLiteral("\U0001F605"),
};

} // namespace

const QStringList& EmojiPicker::emojis() {
  return kEmojis;
}

QString EmojiPicker::emojiAt(int index) {
  if(index < 0 || index >= kEmojis.size())
    return QString();
  return kEmojis.at(index);
}

int EmojiPicker::columns() {
  return kColumns;
}

EmojiPicker::EmojiPicker(QWidget* parent, int* selected, int actionIndex)
    : QTableWidget(parent) {
  setMouseTracking(true);

  setBackgroundRole(QPalette::Window);
  setFrameShape(QFrame::NoFrame);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionMode(QAbstractItemView::NoSelection);
  setShowGrid(false);
  horizontalHeader()->setVisible(false);
  verticalHeader()->setVisible(false);
  setStyleSheet("QTableWidget { padding: 4px }"); // padding around table

  max_col = kColumns;
  int max_row = qCeil(kEmojis.count() / (qreal)max_col);

  setColumnCount(max_col);
  setRowCount(max_row);

  QFont emojiFont = font();
  emojiFont.setPointSize(kFontSizePt);
  //  Emoji-first, base last: color when the color font is installed,
  //  monochrome instead of tofu when it is not.
  emojiFont.setFamilies(ChatHelper::emojiFamiliesWithBase(emojiFont.family()));

  verticalHeader()->setDefaultSectionSize(kCellSize);
  verticalHeader()->setMinimumSectionSize(kCellSize);
  horizontalHeader()->setDefaultSectionSize(kCellSize);
  horizontalHeader()->setMinimumSectionSize(kCellSize);

  //	set min and max size of table, with padding included
  setMinimumSize(max_col * kCellSize + 8, max_row * kCellSize + 8);
  setMaximumSize(max_col * kCellSize + 8, max_row * kCellSize + 8);

  for(int i = 0; i < max_row; i++) {
    for(int j = 0; j < max_col; j++) {
      int k = (i * max_col) + j;

      QTableWidgetItem* item = new QTableWidgetItem();
      item->setData(Qt::UserRole, 0);
      if(k < kEmojis.count()) {
        item->setText(kEmojis.at(k));
        item->setFont(emojiFont);
        item->setTextAlignment(Qt::AlignCenter);
        item->setData(Qt::UserRole, 1);
        item->setBackground(this->palette().window());
      }
      setItem(i, j, item);
    }
  }

  this->actionIndex = actionIndex;
  this->selected = selected;
  this->hoverItem = nullptr;

  connect(this, &QTableWidget::cellClicked, this,
          [this](int row, int column) { pickCell(row, column); });
  connect(this, &QTableWidget::cellActivated, this,
          [this](int row, int column) { pickCell(row, column); });
}

EmojiPicker::~EmojiPicker() {
}

void EmojiPicker::currentChanged(const QModelIndex& current, const QModelIndex& previous) {
  QTableWidget::currentChanged(current, previous);
  (void)previous;
  //  Navigation only moves the highlight; picking is click/Enter in pickCell.
  if(hoverItem)
    viewport()->update(visualItemRect(hoverItem));
  hoverItem = nullptr;
  if(current.isValid()) {
    QTableWidgetItem* cell = item(current.row(), current.column());
    if(cell && cell->data(Qt::UserRole).toInt() == 1) {
      hoverItem = cell;
      viewport()->update(visualItemRect(hoverItem));
    }
  }
}

void EmojiPicker::pickCell(int row, int column) {
  if(picked || !selected)
    return;
  QMenu* pMenu = qobject_cast<QMenu*>(parent());
  if(!pMenu)
    return;
  if(row < 0 || column < 0 || row >= rowCount() || column >= columnCount())
    return;
  QTableWidgetItem* cell = item(row, column);
  if(!cell || cell->data(Qt::UserRole).toInt() != 1)
    return;
  if(actionIndex < 0 || actionIndex >= pMenu->actions().count())
    return;
  picked = true;
  *selected = row * max_col + column;
  pMenu->actions()[actionIndex]->trigger();
  setCurrentIndex(QModelIndex());
  pMenu->close();
}

void EmojiPicker::keyPressEvent(QKeyEvent* e) {
  if((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
     currentIndex().isValid()) {
    pickCell(currentIndex().row(), currentIndex().column());
    return;
  }
  QTableWidget::keyPressEvent(e);
}

void EmojiPicker::mouseMoveEvent(QMouseEvent* e) {
  QTableWidget::mouseMoveEvent(e);

  QTableWidgetItem* currentItem = itemAt(e->position().toPoint());
  if(currentItem != hoverItem) {
    if(hoverItem)
      viewport()->update(visualItemRect(hoverItem));
    hoverItem = currentItem;
    if(hoverItem)
      viewport()->update(visualItemRect(hoverItem));
  }
}

void EmojiPicker::paintEvent(QPaintEvent* e) {
  QTableWidget::paintEvent(e);

  if(hoverItem && !hoverItem->text().isEmpty()) {
    QStyleOptionMenuItem opt;
    opt.initFrom(this);
    opt.rect = visualItemRect(hoverItem);
    opt.state |= QStyle::State_Selected | QStyle::State_Enabled;
    QPainter painter(viewport());
    style()->drawControl(QStyle::CE_MenuItem, &opt, &painter);
    //  Redraw the glyph on top of the highlight.
    painter.setFont(hoverItem->font());
    painter.setPen(palette().color(QPalette::HighlightedText));
    painter.drawText(opt.rect, Qt::AlignCenter, hoverItem->text());
  }
}

void EmojiPicker::leaveEvent(QEvent* e) {
  QTableWidget::leaveEvent(e);

  if(hoverItem)
    viewport()->update(visualItemRect(hoverItem));
  hoverItem = nullptr;
}

void EmojiPicker::showEvent(QShowEvent* e) {
  QTableWidget::showEvent(e);

  //  Widget is reused across popups, rearm the one-shot guard.
  picked = false;
  hoverItem = nullptr;
  setCurrentIndex(QModelIndex());
}
