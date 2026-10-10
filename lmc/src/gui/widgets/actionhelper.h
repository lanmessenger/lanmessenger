/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
**
** Contact:  qualiatech@gmail.com
**
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/


#ifndef ACTIONHELPER_H
#define ACTIONHELPER_H

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

#endif // ACTIONHELPER_H
