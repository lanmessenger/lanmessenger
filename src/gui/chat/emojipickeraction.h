// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QWidgetAction>

class QWidget;

//  Action hosting an EmojiPicker grid; the index is reported via selected.
class EmojiPickerAction : public QWidgetAction {
public:
  EmojiPickerAction(QObject* parent, int* selected);
  ~EmojiPickerAction(void);

  QWidget* createWidget(QWidget* parent) override;

protected:
  void deleteWidget(QWidget* widget) override;

private:
  int* selected;
};
