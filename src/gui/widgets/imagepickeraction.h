// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidget>
#include <QWidgetAction>
#include "imagepicker.h"

class lmcImagePickerAction : public QWidgetAction {
public:
  lmcImagePickerAction(QObject* parent, const QString source[], int sourceCount, int picSize, int columns, int* selected);
  ~lmcImagePickerAction(void);

  void releaseWidget(QWidget* widget);
  QWidget* createWidget(QWidget* parent);

private:
  QList<QString>* source;
  int picSize;
  int columns;
  int* selected;
};
