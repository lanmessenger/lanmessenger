// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
//	This file contains definitions for subclassed controls

#ifndef SUBCONTROLS_H
#define SUBCONTROLS_H

#include <QStylePainter>
#include <QStyleOption>
#include <QToolButton>
#include <QLabel>
#include <QLineEdit>
#include "core/uidefinitions.h"

class lmcToolButton : public QToolButton {
public:
  lmcToolButton(QWidget* parent = 0);

protected:
  void paintEvent(QPaintEvent*);
};

class lmcLabel : public QLabel {
public:
  lmcLabel(QWidget* parent = 0);

  QString text(void) const;
  void setText(const QString& text);

protected:
  void resizeEvent(QResizeEvent*);
  void paintEvent(QPaintEvent*);

private:
  void setElidedText(void);

  QString actualText;
  QString elidedText;
};

class lmcLineEdit : public QLineEdit {
  Q_OBJECT

public:
  lmcLineEdit(QWidget* parent = 0);

signals:
  void lostFocus(void);

protected:
  void focusInEvent(QFocusEvent* event);
  void focusOutEvent(QFocusEvent* event);
};

#endif //SUBCONTROLS_H
