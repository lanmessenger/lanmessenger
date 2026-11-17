// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.

#include <QEvent>
#include <QIcon>
#include <QPixmap>
#include <QStyle>
#include "core/uidefinitions.h"
#include "sendbutton.h"

lmcSendButton::lmcSendButton(QWidget* parent)
    : QToolButton(parent) {
  setFocusPolicy(Qt::NoFocus);
  QSizePolicy policy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  policy.setHorizontalStretch(0);
  policy.setVerticalStretch(0);
  setSizePolicy(policy);
  setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  setIcon(QIcon(QPixmap(IDR_SEND, "PNG")));
  const int extent = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
  setIconSize(QSize(extent, extent));
  updateStyleSheet();
}

void lmcSendButton::updateStyleSheet(void) {
  const QString innerNone = (layoutDirection() == Qt::RightToLeft) ? QStringLiteral("border-right: none;")
                                                                   : QStringLiteral("border-left: none;");
  setStyleSheet(
    QStringLiteral(
      "QToolButton {"
      "border: 1px solid palette(mid);"
      "%1"
      "border-radius: 0px;"
      "padding: 3px 4px;"
      "background-color: transparent;"
      "}"
      "QToolButton:hover {"
      "border: 1px solid palette(highlight);"
      "%1"
      "border-radius: 0px;"
      "background-color: palette(button);"
      "}"
      "QToolButton:pressed {"
      "border: 1px solid palette(dark);"
      "%1"
      "border-radius: 0px;"
      "background-color: palette(midlight);"
      "}"
      "QToolButton:disabled {"
      "border: 1px solid palette(midlight);"
      "%1"
      "border-radius: 0px;"
      "color: palette(mid);"
      "background-color: transparent;"
      "}")
      .arg(innerNone));
}

void lmcSendButton::changeEvent(QEvent* event) {
  switch(event->type()) {
  case QEvent::StyleChange: {
    const int extent = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
    setIconSize(QSize(extent, extent));
    break;
  }
  case QEvent::LayoutDirectionChange:
  case QEvent::LanguageChange:
    updateStyleSheet();
    break;
  default:
    break;
  }
  QToolButton::changeEvent(event);
}
