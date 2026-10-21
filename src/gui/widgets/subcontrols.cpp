// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QApplication>
#include "core/lmcstrings.h"
#include "subcontrols.h"

lmcToolButton::lmcToolButton(QWidget* parent)
    : QToolButton(parent) {
}

void lmcToolButton::paintEvent(QPaintEvent*) {
  QStylePainter p(this);
  QStyleOptionToolButton opt;
  initStyleOption(&opt);
  opt.features &= (~QStyleOptionToolButton::HasMenu);
  p.drawComplexControl(QStyle::CC_ToolButton, opt);
}

lmcLabel::lmcLabel(QWidget* parent)
    : QLabel(parent) {
  actualText = elidedText = QString();
}

QString lmcLabel::text(void) const {
  return actualText;
}

void lmcLabel::setText(const QString& text) {
  actualText = text;
  setElidedText();
}

void lmcLabel::resizeEvent(QResizeEvent*) {
  setElidedText();
}

void lmcLabel::paintEvent(QPaintEvent*) {
  QStylePainter p(this);
  p.drawText(rect(), alignment(), elidedText);
}

void lmcLabel::setElidedText(void) {
  elidedText = fontMetrics().elidedText(actualText, Qt::ElideRight, width());
}

lmcLineEdit::lmcLineEdit(QWidget* parent)
    : QLineEdit(parent) {
  QPalette p = QApplication::palette();
  p.setColor(QPalette::Base, p.color(QPalette::Window));
  p.setColor(QPalette::Text, GRAY_TEXT_COLOR);
  setPalette(p);
}

void lmcLineEdit::focusInEvent(QFocusEvent* event) {
  QLineEdit::focusInEvent(event);

  setSelection(0, text().length());

  QPalette p = QApplication::palette();
  p.setColor(QPalette::Base, p.color(QPalette::Base));
  p.setColor(QPalette::Text, p.color(QPalette::Text));
  setPalette(p);
}

void lmcLineEdit::focusOutEvent(QFocusEvent* event) {
  QLineEdit::focusOutEvent(event);

  QPalette p = QApplication::palette();
  p.setColor(QPalette::Base, p.color(QPalette::Window));
  p.setColor(QPalette::Text, GRAY_TEXT_COLOR);
  setPalette(p);

  emit lostFocus();
}
