// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.

#include "infobar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QSizePolicy>
#include <QtGlobal>

lmcInfoBar::lmcInfoBar(QWidget* parent)
    : QFrame(parent) {
  setFrameShape(QFrame::NoFrame);

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 2, 8, 2);

  pTextLabel = new QLabel(this);
  pTextLabel->setTextFormat(Qt::PlainText);
  pTextLabel->setWordWrap(true);
  pTextLabel->setAlignment(Qt::AlignCenter);
  pTextLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  pTextLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  layout->addWidget(pTextLabel);

  clear();
}

void lmcInfoBar::showMessage(Severity newSeverity, const QString& text) {
  if(text.isEmpty()) {
    clear();
    return;
  }
  severity = newSeverity;
  pTextLabel->setText(text);
  update();
  setVisible(true);
}

void lmcInfoBar::clear(void) {
  severity = Info;
  pTextLabel->setText(QString());
  setVisible(false);
}

void lmcInfoBar::paintEvent(QPaintEvent* event) {
  {
    QPainter painter(this);
    painter.fillRect(rect(), blendedBackground());
    painter.setPen(palette().color(QPalette::Mid));
    const int y = rect().bottom();
    painter.drawLine(rect().left(), y, rect().right(), y);
  }
  QFrame::paintEvent(event);
}

void lmcInfoBar::changeEvent(QEvent* event) {
  switch(event->type()) {
  case QEvent::StyleChange:
  case QEvent::PaletteChange:
    update();
    break;
  default:
    break;
  }
  QFrame::changeEvent(event);
}

QColor lmcInfoBar::blendedBackground(void) const {
  const QColor window = palette().color(QPalette::Window);
  const QColor tint = tintFor(severity);
  const qreal amount = 0.15;
  return QColor(qRound(window.red() + (tint.red() - window.red()) * amount),
                qRound(window.green() + (tint.green() - window.green()) * amount),
                qRound(window.blue() + (tint.blue() - window.blue()) * amount));
}

QColor lmcInfoBar::tintFor(Severity severity) {
  switch(severity) {
  case Warning:
    return QColor(255, 115, 0);
  case Error:
    return QColor(192, 0, 0);
  case Info:
  default:
    return QColor(128, 128, 128);
  }
}
