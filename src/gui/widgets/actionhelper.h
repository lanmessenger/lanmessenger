// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QAction>
#include <QIcon>
#include <QKeySequence>
#include <QObject>
#include <QString>
#include <QWidget>

class ActionHelper {
public:
  // Replacement for the QMenu/QToolBar/QWidget addAction() convenience overloads
  // taking a pointer-to-member/functor (Qt >= 6.4 only). Our CI baseline is Qt 6.2
  // (Ubuntu 22.04), where those overloads do not exist, so a connected action is
  // built explicitly: construct, optionally set shortcut, connect, add.
  template <typename Receiver, typename Slot>
  static QAction* createAction(QWidget* parent, const QString& text, Receiver* receiver, Slot slot,
                               const QIcon& icon = QIcon(),
                               const QKeySequence& shortcut = QKeySequence()) {
    QAction* action = new QAction(icon, text, parent);
    if(!shortcut.isEmpty())
      action->setShortcut(shortcut);
    QObject::connect(action, &QAction::triggered, receiver, slot);
    parent->addAction(action);
    return action;
  }
};
