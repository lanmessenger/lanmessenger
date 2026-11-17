// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.

#pragma once

#include <QColor>
#include <QEvent>
#include <QFrame>
#include <QPaintEvent>
#include <QString>

class QLabel;

//	Shared status banner (lblInfo) for chat/chatroom/broadcast windows.
class lmcInfoBar : public QFrame {
public:
  enum Severity {
    Info,
    Warning,
    Error
  };

  explicit lmcInfoBar(QWidget* parent = nullptr);

  void showMessage(Severity newSeverity, const QString& text);
  void clear(void);

protected:
  void paintEvent(QPaintEvent* event) override;
  void changeEvent(QEvent* event) override;

private:
  QColor blendedBackground(void) const;
  static QColor tintFor(Severity severity);

  QLabel* pTextLabel;
  Severity severity = Info;
};
