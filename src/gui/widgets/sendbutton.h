// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.

#pragma once

#include <QToolButton>

class QEvent;

class lmcSendButton : public QToolButton {
public:
  explicit lmcSendButton(QWidget* parent = nullptr);

protected:
  void changeEvent(QEvent* event) override;

private:
  void updateStyleSheet(void);
};
