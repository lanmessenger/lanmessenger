// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include <QMenu>
#include <QWidget>
#include "emojipickeraction.h"
#include "emojipicker.h"

EmojiPickerAction::EmojiPickerAction(QObject* parent, int* selected)
    : QWidgetAction(parent) {
  this->selected = selected;
}

EmojiPickerAction::~EmojiPickerAction(void) {
}

void EmojiPickerAction::deleteWidget(QWidget* widget) {
  widget->deleteLater();
}

QWidget* EmojiPickerAction::createWidget(QWidget* parent) {
  QMenu* menu = qobject_cast<QMenu*>(parent);
  if(!menu)
    return nullptr;
  int index = 0;
  for(; index < menu->actions().count(); index++)
    if(menu->actions()[index] == this)
      break;
  if(index >= menu->actions().count())
    return nullptr;
  EmojiPicker* widget = new EmojiPicker(parent, selected, index);
  return widget;
}
