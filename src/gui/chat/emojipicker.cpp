// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include <QMenu>
#include <QHeaderView>
#include <QPainter>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QShowEvent>
#include <qmath.h>
#include "emojipicker.h"
#include "gui/chat/chathelper.h"

namespace {

const int kColumns = 10;
const int kCellSize = 32;
const int kCellMargin = 8;
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
  QStringLiteral("\U0001F603"),
  QStringLiteral("\U0001F604"),
  QStringLiteral("\U0001F606"),
  QStringLiteral("\U0001F607"),
  QStringLiteral("\U0001F60B"),
  QStringLiteral("\U0001F60F"),
  QStringLiteral("\U0001F612"),
  QStringLiteral("\U0001F613"),
  QStringLiteral("\U0001F616"),
  QStringLiteral("\U0001F61E"),
  QStringLiteral("\U0001F62D"),
  QStringLiteral("\U0001F631"),
  QStringLiteral("\U0001F917"),
  QStringLiteral("\U0001F92A"),
  QStringLiteral("\U0001F608"),
  QStringLiteral("\U0001F636"),
  QStringLiteral("\U0001F64C"),
  QStringLiteral("\U0001F91D"),
  QStringLiteral("\U0001F44B"),
  QStringLiteral("\U0001F446"),
  QStringLiteral("\U0001F447"),
  QStringLiteral("\U0001F449"),
  QStringLiteral("\U0001F91E"),
  QStringLiteral("\U0001F918"),
  QStringLiteral("\u270C\uFE0F"),
  QStringLiteral("\U0001F63A"),
  QStringLiteral("\U0001F495"),
  QStringLiteral("\U0001F49C"),
  QStringLiteral("\u2B50\uFE0F"),
  QStringLiteral("\U0001F31F"),
  QStringLiteral("\U0001F308"),
  QStringLiteral("\u2600\uFE0F"),
  QStringLiteral("\u2615\uFE0F"),
  QStringLiteral("\u2708\uFE0F"),
  QStringLiteral("\u26BD\uFE0F"),
  QStringLiteral("\u23F0\uFE0F"),
  QStringLiteral("\U0001F339"),
  QStringLiteral("\U0001F37A"),
  QStringLiteral("\U0001F382"),
  QStringLiteral("\U0001F381"),
  QStringLiteral("\U0001F4F7"),
  QStringLiteral("\U0001F4A1"),
  QStringLiteral("\U0001F4BB"),
  QStringLiteral("\U0001F697"),
  QStringLiteral("\U0001F4A4"),
  QStringLiteral("\U0001F340"),
  QStringLiteral("\U0001F514"),
  QStringLiteral("\U0001F3B5"),
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

EmojiCellDelegate::EmojiCellDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

void EmojiCellDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
  QStyleOptionViewItem opt(option);
  initStyleOption(&opt, index);
  const bool hasEmoji = index.data(Qt::UserRole).toInt() == 1;
  QPalette::ColorRole bgRole = QPalette::Window;
  QPalette::ColorRole fgRole = QPalette::Text;
  if(hasEmoji &&
     (opt.state &
      (QStyle::State_Selected | QStyle::State_MouseOver | QStyle::State_HasFocus))) {
    bgRole = QPalette::Highlight;
    fgRole = QPalette::HighlightedText;
  }
  painter->fillRect(opt.rect, opt.palette.color(bgRole));
  if(!hasEmoji)
    return;
  //  Never elided: the cell fits the widest glyph by construction.
  painter->save();
  painter->setFont(opt.font);
  painter->setPen(opt.palette.color(fgRole));
  painter->drawText(opt.rect, Qt::AlignCenter, opt.text);
  painter->restore();
}

int EmojiPicker::cellSize(const QFont& emojiFont) {
  const QFontMetrics metrics(emojiFont);
  int need = metrics.height();
  for(const QString& emoji : kEmojis)
    need = qMax(need, metrics.horizontalAdvance(emoji));
  return qMax(kCellSize, need + kCellMargin);
}

void EmojiPicker::updateCellSize() {
  QFont emojiFont = font();
  emojiFont.setPointSize(kFontSizePt);
  //  Emoji-first, base last: color when the color font is installed,
  //  monochrome instead of tofu when it is not.
  emojiFont.setFamilies(ChatHelper::emojiFamiliesWithBase(emojiFont.family()));

  const int cell = cellSize(emojiFont);
  const int max_row = qCeil(kEmojis.count() / (qreal)max_col);
  verticalHeader()->setDefaultSectionSize(cell);
  verticalHeader()->setMinimumSectionSize(cell);
  horizontalHeader()->setDefaultSectionSize(cell);
  horizontalHeader()->setMinimumSectionSize(cell);

  setMinimumSize(max_col * cell, max_row * cell);
  setMaximumSize(max_col * cell, max_row * cell);

  for(int i = 0; i < rowCount(); i++)
    for(int j = 0; j < columnCount(); j++)
      if(QTableWidgetItem* cellItem = item(i, j))
        if(cellItem->data(Qt::UserRole).toInt() == 1)
          cellItem->setFont(emojiFont);
}

EmojiPicker::EmojiPicker(QWidget* parent, int* selected, int actionIndex)
    : QTableWidget(parent) {
  setMouseTracking(true);
  viewport()->setMouseTracking(true);

  setFrameShape(QFrame::NoFrame);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionMode(QAbstractItemView::NoSelection);
  setTextElideMode(Qt::ElideNone);
  setShowGrid(false);
  horizontalHeader()->setVisible(false);
  verticalHeader()->setVisible(false);
  setStyleSheet(QStringLiteral("QTableWidget::item { padding: 0px; margin: 0px; border: 0px; }"));
  setItemDelegate(new EmojiCellDelegate(this));

  max_col = kColumns;
  int max_row = qCeil(kEmojis.count() / (qreal)max_col);

  setColumnCount(max_col);
  setRowCount(max_row);

  for(int i = 0; i < max_row; i++) {
    for(int j = 0; j < max_col; j++) {
      int k = (i * max_col) + j;

      QTableWidgetItem* item = new QTableWidgetItem();
      item->setData(Qt::UserRole, 0);
      if(k < kEmojis.count()) {
        item->setText(kEmojis.at(k));
        item->setTextAlignment(Qt::AlignCenter);
        item->setData(Qt::UserRole, 1);
      }
      setItem(i, j, item);
    }
  }
  updateCellSize();

  this->actionIndex = actionIndex;
  this->selected = selected;

  connect(this, &QTableWidget::cellClicked, this,
          [this](int row, int column) { pickCell(row, column); });
  connect(this, &QTableWidget::cellActivated, this,
          [this](int row, int column) { pickCell(row, column); });
}

EmojiPicker::~EmojiPicker() {
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

void EmojiPicker::showEvent(QShowEvent* e) {
  QTableWidget::showEvent(e);

  //  Widget is reused across popups, rearm the one-shot guard.
  picked = false;
  updateCellSize();
  setCurrentIndex(QModelIndex());
}
