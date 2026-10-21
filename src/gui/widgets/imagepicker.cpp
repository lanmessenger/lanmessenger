// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QMenu>
#include <QHeaderView>
#include <QPainter>
#include <QMouseEvent>
#include <qmath.h>
#include "core/uidefinitions.h"
#include "imagepicker.h"

lmcImagePicker::lmcImagePicker(QWidget* parent, QList<QString>* source, int picSize, int columns, int* selected, int actionIndex)
    : QTableWidget(parent) {
  setMouseTracking(true);

  setBackgroundRole(QPalette::Window);
  setIconSize(QSize(picSize, picSize));
  setFrameShape(QFrame::NoFrame);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionMode(QAbstractItemView::NoSelection);
  setShowGrid(false);
  horizontalHeader()->setVisible(false);
  verticalHeader()->setVisible(false);
  setStyleSheet("QTableWidget { padding: 4px }"); // padding around table

  max_col = columns;
  int max_row = qCeil(source->count() / (qreal)max_col);

  setColumnCount(max_col);
  setRowCount(max_row);

  int cellSize = picSize + 8;
  verticalHeader()->setDefaultSectionSize(cellSize);
  verticalHeader()->setMinimumSectionSize(cellSize);
  horizontalHeader()->setDefaultSectionSize(cellSize);
  horizontalHeader()->setMinimumSectionSize(cellSize);

  //	set min and max size of table, with padding included
  setMinimumSize(max_col * cellSize + 8, max_row * cellSize + 8);
  setMaximumSize(max_col * cellSize + 8, max_row * cellSize + 8);

  for(int i = 0; i < max_row; i++) {
    for(int j = 0; j < max_col; j++) {
      int k = (i * max_col) + j;

      QTableWidgetItem* item = new QTableWidgetItem();
      item->setData(TypeRole, 0);
      if(k < source->count()) {
        item->setIcon(QIcon(source->value(k)));
        item->setData(TypeRole, 1);
        item->setSizeHint(QSize(picSize, picSize));
        item->setBackground(this->palette().window());
      }
      setItem(i, j, item);
    }
  }

  this->actionIndex = actionIndex;
  this->selected = selected;
  this->hoverItem = NULL;
}

lmcImagePicker::~lmcImagePicker() {
}

void lmcImagePicker::currentChanged(const QModelIndex& current, const QModelIndex& previous) {
  QMenu* pMenu = (QMenu*)this->parent();
  if(current.isValid()) {
    int index = current.row() * max_col + current.column();
    if(current.data(TypeRole).toInt() == 1) {
      *selected = index;
      pMenu->actions()[actionIndex]->trigger();
    } else {
      setCurrentIndex(previous);
      return;
    }
  }
  hoverItem = NULL;
  setCurrentIndex(QModelIndex());
  pMenu->close();
}

void lmcImagePicker::mouseMoveEvent(QMouseEvent* e) {
  QTableWidget::mouseMoveEvent(e);

  QTableWidgetItem* currentItem = itemAt(e->position().toPoint());
  if(currentItem != hoverItem) {
    hoverItem = currentItem;
    if(hoverItem)
      update(visualItemRect(hoverItem));
  }
}

void lmcImagePicker::paintEvent(QPaintEvent* e) {
  QTableWidget::paintEvent(e);

  //	If mouse is hovered over an item, draw a border around it
  if(hoverItem) {
    QStyleOptionFrame opt;
    opt.rect = visualItemRect(hoverItem);
    opt.rect.adjust(0, 1, -2, -1);
    QPainter painter(viewport());
    style()->drawPrimitive(QStyle::PE_FrameButtonBevel, &opt, &painter);
  }
}

void lmcImagePicker::leaveEvent(QEvent* e) {
  QTableWidget::leaveEvent(e);

  hoverItem = NULL;
}