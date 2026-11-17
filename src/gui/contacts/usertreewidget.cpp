// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "usertreewidget.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

lmcUserTreeWidgetItem::lmcUserTreeWidgetItem()
    : QTreeWidgetItem(UserType + 1) {
  //	make item not user checkable
  setFlags(flags() & ~Qt::ItemIsUserCheckable);
}

QRect lmcUserTreeWidgetItem::checkBoxRect(const QRect& itemRect) {
  lmcUserTreeWidget* treeWidget = static_cast<lmcUserTreeWidget*>(this->treeWidget());
  int cbWidth = 12;
  int cbHeight = 12;
  if(treeWidget) {
    QStyle* style = treeWidget->style();
    cbWidth = style->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, treeWidget);
    cbHeight = style->pixelMetric(QStyle::PM_IndicatorHeight, nullptr, treeWidget);
    //	Clamp to the row height so a large accessibility theme can never overflow the row.
    cbWidth = qMin(cbWidth, itemRect.height());
    cbHeight = qMin(cbHeight, itemRect.height());
  }

  QRect checkBoxRect(0, 0, 0, 0);

  QString itemType = data(0, TypeRole).toString();
  if(itemType == "Group")
    checkBoxRect = QRect(itemRect.left(), itemRect.top() + ((itemRect.height() - cbHeight) / 2), 0, 0);
  else if(itemType == "User")
    //	Top-aligned to sit on the same line as the status icon (top + padding), not centered.
    checkBoxRect = QRect(itemRect.left(), itemRect.top() + qBound(0, itemRect.height() - cbHeight, 2), 0, 0);

  if(treeWidget && treeWidget->checkable()) {
    checkBoxRect.setSize(QSize(cbWidth, cbHeight));
    checkBoxRect.moveLeft(checkBoxRect.left() + 3);
  }

  return checkBoxRect;
}

void lmcUserTreeWidgetGroupItem::addChild(QTreeWidgetItem* child) {
  lmcUserTreeWidget* treeWidget = static_cast<lmcUserTreeWidget*>(this->treeWidget());
  child->setSizeHint(0, QSize(0, itemViewHeight[treeWidget->view()]));
  QTreeWidgetItem::addChild(child);
}

bool lmcUserTreeWidgetUserItem::operator<(const QTreeWidgetItem& other) const {
  int column = treeWidget()->sortColumn();
  if(column == 0) {
    //	sort based on status and user name
    if(data(column, StatusRole).toInt() < other.data(column, StatusRole).toInt())
      return true;
    else if(data(column, StatusRole).toInt() > other.data(column, StatusRole).toInt())
      return false;
    else
      return text(column).toLower() < other.text(column).toLower();
  }

  return text(column).toLower() < other.text(column).toLower();
}

void lmcUserTreeWidgetDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  painter->save();

  const QPalette& palette = option.palette;
  bool isEnabled = option.state.testFlag(QStyle::State_Enabled);
  bool isActive = option.state.testFlag(QStyle::State_Active);
  QPalette::ColorGroup group = !isEnabled ? QPalette::Disabled : (isActive ? QPalette::Active : QPalette::Inactive);
  QRect itemRect = option.rect;
  lmcUserTreeWidgetItem* pItem = static_cast<lmcUserTreeWidgetItem*>(index.internalPointer());
  if(!pItem) {
    painter->restore();
    QStyledItemDelegate::paint(painter, option, index);
    return;
  }
  lmcUserTreeWidget* pTreeWidget = static_cast<lmcUserTreeWidget*>(pItem->treeWidget());
  if(!pTreeWidget) {
    painter->restore();
    QStyledItemDelegate::paint(painter, option, index);
    return;
  }
  QString type = pItem->data(0, TypeRole).toString();
  QString name = pItem->data(0, Qt::DisplayRole).toString();
  bool isSelected = option.state.testFlag(QStyle::State_Selected);
  const QWidget* widget = option.widget;
  QStyle* style = widget ? widget->style() : QApplication::style();

  int padding = 2;

  //	Native background (selection, hover, alternate, disabled) via style.
  QStyleOptionViewItem backgroundOpt(option);
  backgroundOpt.palette.setCurrentColorGroup(group);
  backgroundOpt.text = QString();
  backgroundOpt.icon = QIcon();
  if(type == "Group")
    backgroundOpt.features.setFlag(QStyleOptionViewItem::Alternate, false);
  style->drawPrimitive(QStyle::PE_PanelItemViewItem, &backgroundOpt, painter, widget);

  if(type == "Group") {
    //	Section header: neutral background from style, bold text, bottom separator.
    if(!isSelected) {
      painter->setPen(QPen(palette.color(group, QPalette::Mid)));
      painter->drawLine(itemRect.left(), itemRect.bottom(), itemRect.right(), itemRect.bottom());
    }

    //	Draw checkbox if needed (native indicator)
    QRect checkBoxRect = pItem->checkBoxRect(itemRect);
    if(pTreeWidget->checkable())
      drawCheckBox(painter, option, group, checkBoxRect, pItem->checkState(0));

    //	Draw the expand/collapse arrow via style. Qt has no dedicated branch-width metric
    //	(SE_TreeViewDisclosureItem returns the full item rect), so use the DPI-aware
    //	indicator width which matches the checkbox alignment.
    QStyleOptionViewItem branchOpt(option);
    int branchWidth = style->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, widget);
    if(branchWidth <= 0)
      branchWidth = 12;
    QRect branchRect(checkBoxRect.right() + 4, itemRect.top(), branchWidth, itemRect.height());
    branchOpt.rect = branchRect;
    branchOpt.palette.setCurrentColorGroup(group);
    style->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOpt, painter, widget);

    //	Draw the text
    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    int textFlags = Qt::AlignLeft | Qt::AlignVCenter;
    //	Leave a padding of 5px on left and right; clamp so a bad metric can never push the name out.
    int leftPad = branchRect.right() + 4 - itemRect.left();
    int maxPad = itemRect.width() - 5 - 24;
    if(maxPad < padding)
      maxPad = padding;
    leftPad = qBound(padding, leftPad, maxPad);
    QRect textRect = itemRect.adjusted(leftPad, padding, -5, -padding);
    if(isSelected)
      painter->setPen(QPen(palette.color(group, QPalette::HighlightedText)));
    else
      painter->setPen(QPen(palette.color(group, QPalette::WindowText)));
    QString text = painter->fontMetrics().elidedText(name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, textFlags, text);
  } else if(type == "User") {
    //	Draw checkbox if needed (native indicator)
    QRect checkBoxRect = pItem->checkBoxRect(itemRect);
    if(pTreeWidget->checkable())
      drawCheckBox(painter, option, group, checkBoxRect, pItem->checkState(0));

    //	Draw the status image
    QPixmap statusImage = pItem->icon(0).pixmap(QSize(16, 16));
    int leftPad = checkBoxRect.width() > 0 ? checkBoxRect.right() + 5 : 5;
    QRect statusRect = itemRect.adjusted(leftPad, padding, 0, 0);
    statusRect.setSize(statusImage.size());
    painter->drawPixmap(statusRect, statusImage);

    //	Draw the avatar image
    QRect avatarRect = itemRect.adjusted(itemRect.width(), padding, 0, 0);
    if(pTreeWidget->view() == ULV_Detailed) {
      QVariant avatar = pItem->data(0, AvatarRole);
      if(!avatar.isNull()) {
        QPixmap avatarImage = ((QIcon)pItem->data(0, AvatarRole).value<QIcon>()).pixmap(32, 32);
        avatarRect.setLeft(avatarRect.right() - avatarImage.width() - padding);
        avatarRect.setSize(avatarImage.size());
        painter->drawPixmap(avatarRect, avatarImage);
      }
    }

    //	Draw the text
    if(isSelected)
      painter->setPen(QPen(palette.color(group, QPalette::HighlightedText)));
    else
      painter->setPen(QPen(palette.color(group, QPalette::WindowText)));
    int textFlags = Qt::AlignLeft;
    textFlags |= (pTreeWidget->view() == ULV_Detailed ? Qt::AlignTop : Qt::AlignVCenter);
    //	Leave a padding of 5px on left and right; clamp so a narrow viewport can never hide the name.
    int rightReserve = 5 + avatarRect.width() + padding;
    int textLeft = statusRect.right() + 5 - itemRect.left();
    int maxLeft = itemRect.width() - 5 - rightReserve - 24;
    if(maxLeft < padding)
      maxLeft = padding;
    textLeft = qBound(padding, textLeft, maxLeft);
    QRect textRect = itemRect.adjusted(textLeft, padding, -rightReserve, -padding);
    QString text = painter->fontMetrics().elidedText(name, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, textFlags, text);

    //	Draw sub text
    if(pTreeWidget->view() == ULV_Detailed) {
      QVariant note = pItem->data(0, SubtextRole);
      if(!note.isNull()) {
        QString userNote = note.toString();
        if(isSelected)
          painter->setPen(QPen(palette.color(group, QPalette::HighlightedText)));
        else
          painter->setPen(QPen(palette.color(group, QPalette::PlaceholderText)));
        textFlags = Qt::AlignLeft | Qt::AlignBottom;
        text = painter->fontMetrics().elidedText(userNote, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, textFlags, text);
      }
    }
  }

  if(option.state.testFlag(QStyle::State_HasFocus)) {
    QStyleOptionFocusRect focusOpt;
    focusOpt.rect = itemRect;
    focusOpt.state = option.state;
    focusOpt.palette = palette;
    focusOpt.palette.setCurrentColorGroup(group);
    focusOpt.backgroundColor = palette.color(group, isSelected ? QPalette::Highlight : QPalette::Window);
    style->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOpt, painter, widget);
  }

  painter->restore();
}

void lmcUserTreeWidgetDelegate::drawCheckBox(QPainter* painter, const QStyleOptionViewItem& option,
                                             QPalette::ColorGroup group, const QRect& checkBoxRect,
                                             Qt::CheckState checkState) const {
  QStyleOptionButton checkOpt;
  checkOpt.rect = checkBoxRect;
  checkOpt.palette = option.palette;
  checkOpt.palette.setCurrentColorGroup(group);
  checkOpt.direction = option.direction;
  checkOpt.fontMetrics = option.fontMetrics;
  checkOpt.styleObject = option.styleObject;
  checkOpt.state = QStyle::State_Enabled;
  if(!option.state.testFlag(QStyle::State_Enabled))
    checkOpt.state = QStyle::State_None;
  checkOpt.state |= (option.state & (QStyle::State_MouseOver | QStyle::State_HasFocus | QStyle::State_Selected |
                                     QStyle::State_Sunken | QStyle::State_Active));
  if(checkState == Qt::Checked)
    checkOpt.state |= QStyle::State_On;
  else if(checkState == Qt::PartiallyChecked)
    checkOpt.state |= QStyle::State_NoChange;
  else
    checkOpt.state |= QStyle::State_Off;
  const QWidget* widget = option.widget;
  QStyle* style = widget ? widget->style() : QApplication::style();
  style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkOpt, painter, widget);
}

lmcUserTreeWidget::lmcUserTreeWidget(QWidget* parent)
    : QTreeWidget(parent) {
  itemDelegate = new lmcUserTreeWidgetDelegate();
  setItemDelegate(itemDelegate);
  setAlternatingRowColors(true);

  isCheckable = false;
  viewType = ULV_Detailed;
}

bool lmcUserTreeWidget::checkable(void) {
  return isCheckable;
}

void lmcUserTreeWidget::setCheckable(bool enable) {
  isCheckable = enable;
}

UserListView lmcUserTreeWidget::view(void) {
  return viewType;
}

void lmcUserTreeWidget::setView(UserListView view) {
  viewType = view;

  //	Set the item heights for the selected view type
  for(int index = 0; index < topLevelItemCount(); index++) {
    QTreeWidgetItem* item = topLevelItem(index);
    for(int childIndex = 0; childIndex < item->childCount(); childIndex++)
      item->child(childIndex)->setSizeHint(0, QSize(0, itemViewHeight[viewType]));
  }
}

void lmcUserTreeWidget::mousePressEvent(QMouseEvent* event) {
  if(event->button() == Qt::LeftButton) {
    QTreeWidgetItem* item = itemAt(event->position().toPoint());

    dragGroup = false;
    dragUser = false;
    dragItem = NULL;
    parentId = QString();
    expanded = false;

    if(item) {
      dragItem = item;
      if(dynamic_cast<lmcUserTreeWidgetGroupItem*>(item)) {
        dragGroup = true;
        expanded = dragItem->isExpanded();
      } else if(dynamic_cast<lmcUserTreeWidgetUserItem*>(item)) {
        dragUser = true;
        parentId = dragItem->parent()->data(0, IdRole).toString();
      }
    }
  }

  QTreeWidget::mousePressEvent(event);
}

void lmcUserTreeWidget::dragMoveEvent(QDragMoveEvent* event) {
  QTreeWidget::dragMoveEvent(event);

  QTreeWidgetItem* item = itemAt(event->position().toPoint());
  bool accept = false;

  if(dragUser) {
    if(item && dynamic_cast<lmcUserTreeWidgetGroupItem*>(item) && visualItemRect(item).contains(event->position().toPoint(), true))
      accept = true;
  } else if(dragGroup) {
    if(!item || (dynamic_cast<lmcUserTreeWidgetGroupItem*>(item) && !visualItemRect(item).contains(event->position().toPoint(), true)))
      accept = true;
  }

  accept ? event->accept() : event->ignore();
}

void lmcUserTreeWidget::dropEvent(QDropEvent* event) {
  QTreeWidget::dropEvent(event);

  if(dragUser) {
    if(!dragItem->parent()) {
      //	user item dragged to group level. revert
      for(int index = 0; index < topLevelItemCount(); index++) {
        QTreeWidgetItem* parentItem = topLevelItem(index);
        if(parentItem->data(0, IdRole).toString().compare(parentId) == 0) {
          takeTopLevelItem(indexOfTopLevelItem(dragItem));
          parentItem->addChild(dragItem);
          return;
        }
      }
    }
  } else if(dragGroup) {
    dragItem->setExpanded(expanded);
  }

  if(dragItem)
    emit itemDragDropped(dragItem);
}

void lmcUserTreeWidget::contextMenuEvent(QContextMenuEvent* event) {
  QTreeWidget::contextMenuEvent(event);

  QTreeWidgetItem* item = itemAt(event->pos());
  QPoint pos = event->globalPos();
  if(item && event->reason() != QContextMenuEvent::Mouse) {
    QRect itemRect = visualItemRect(item);
    pos = itemRect.bottomLeft();
    pos.ry() += itemRect.height();
    pos = mapToGlobal(pos);
  }

  emit itemContextMenu(item, pos);
}

void lmcUserTreeWidget::mouseReleaseEvent(QMouseEvent* event) {
  QTreeWidget::mouseReleaseEvent(event);

  QPoint pos = event->position().toPoint();
  lmcUserTreeWidgetItem* item = static_cast<lmcUserTreeWidgetItem*>(itemAt(pos));
  if(item && checkable() && item->checkBoxRect(visualItemRect(item)).contains(pos)) {
    // toggle checkstate
    if(item->checkState(0) == Qt::Checked)
      item->setCheckState(0, Qt::Unchecked);
    else
      item->setCheckState(0, Qt::Checked);
  }
}

void lmcUserTreeWidget::keyPressEvent(QKeyEvent* event) {
  QTreeWidget::keyPressEvent(event);

  if(event->key() == Qt::Key_Space && selectedItems().count() > 0) {
    lmcUserTreeWidgetItem* item = static_cast<lmcUserTreeWidgetItem*>(selectedItems().at(0));
    if(item && checkable()) {
      // toggle checkstate
      if(item->checkState(0) == Qt::Checked)
        item->setCheckState(0, Qt::Unchecked);
      else
        item->setCheckState(0, Qt::Checked);
    }
  }
}
