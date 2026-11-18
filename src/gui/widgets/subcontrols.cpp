// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
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
  applyUnfocusedLook();
}

void lmcLineEdit::focusInEvent(QFocusEvent* event) {
  QLineEdit::focusInEvent(event);

  setPalette(QApplication::palette());
  setFrame(true);
}

void lmcLineEdit::focusOutEvent(QFocusEvent* event) {
  QLineEdit::focusOutEvent(event);

  applyUnfocusedLook();

  emit lostFocus();
}

void lmcLineEdit::changeEvent(QEvent* event) {
  QLineEdit::changeEvent(event);

  switch(event->type()) {
  case QEvent::PaletteChange:
  case QEvent::ApplicationPaletteChange:
  case QEvent::StyleChange:
    if(hasFocus())
      setPalette(QApplication::palette());
    else
      applyUnfocusedLook();
    break;
  default:
    break;
  }
}

void lmcLineEdit::enterEvent(QEnterEvent* event) {
  QLineEdit::enterEvent(event);

  if(!isEnabled() || hasFocus())
    return;
  setFrame(true);
}

void lmcLineEdit::leaveEvent(QEvent* event) {
  QLineEdit::leaveEvent(event);

  if(!hasFocus())
    setFrame(false);
}

void lmcLineEdit::applyUnfocusedLook(void) {
  QPalette pal = QApplication::palette();
  const QColor window = pal.color(QPalette::Window);
  const QColor foreground = pal.color(QPalette::WindowText);
  constexpr qreal amount = 0.25;
  const QColor soft(qRound(foreground.red() + (window.red() - foreground.red()) * amount),
                    qRound(foreground.green() + (window.green() - foreground.green()) * amount),
                    qRound(foreground.blue() + (window.blue() - foreground.blue()) * amount));
  pal.setColor(QPalette::Base, window);
  pal.setColor(QPalette::Text, soft);
  setPalette(pal);
  setFrame(underMouse());
}
