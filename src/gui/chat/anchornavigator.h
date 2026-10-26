// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QPoint>
#include <QRect>
#include <QStringList>

class QTextDocument;

//  Pure anchor navigation over a rendered QTextDocument.
//  docRect is already in view coordinates (translated by the caller).
class AnchorNavigator {
public:
  static QString anchorAt(const QTextDocument* doc, const QRect& docRect, const QPoint& pos);
  static QRect anchorRect(const QTextDocument* doc, const QRect& docRect, const QString& href);
  static QStringList anchors(const QTextDocument* doc);
};
