// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QMenu>
#include "imagepickeraction.h"

lmcImagePickerAction::lmcImagePickerAction(QObject* parent, const QString source[], int sourceCount, int picSize, int columns, int* selected)
    : QWidgetAction(parent) {
  this->source = new QList<QString>();
  for(int index = 0; index < sourceCount; index++)
    this->source->append(source[index]);
  this->picSize = picSize;
  this->columns = columns;
  this->selected = selected;
}

lmcImagePickerAction::~lmcImagePickerAction(void) {
}

void lmcImagePickerAction::releaseWidget(QWidget* widget) {
  widget->deleteLater();
}

QWidget* lmcImagePickerAction::createWidget(QWidget* parent) {
  QMenu* menu = (QMenu*)parent;
  int index = 0;
  for(; index < menu->actions().count(); index++)
    if(menu->actions()[index] == this)
      break;
  lmcImagePicker* widget = new lmcImagePicker(parent, source, picSize, columns, selected, index);
  return widget;
}