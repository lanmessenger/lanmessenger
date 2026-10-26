// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QString>
#include <QStringList>

#include "core/theme.h"

//  One chat row resolved from a compiled layout_* tree.
//  Card is one of incoming/outgoing/system/broadcast/request;
//  in-series rows use the _next tree (no header/avatar there).
struct ResolvedRow {
  QString card;
  bool avatar = false;
  bool header = false;
  QStringList headerItems;
  int hboxGap = 8;
  int vboxGap = 2;
  Theme::DualColor text;        // body text: node value or theme default
  Theme::DualColor senderColor; // header name: node value or theme default
  Theme::DualColor timeColor;   // header time: node value or theme default
  Theme::DualColor border;      // bubble outline
  bool hasBorder = false;
  Theme::Bg bg;
  int padding[4] = {0, 0, 0, 0}; // [T, R, B, L]
  double shadowDx = 0.0;
  double shadowDy = 0.0;
  double shadowBlur = 0.0;
  double shadowOpacity = 0.0;
  int marginTop = 0;     // row top gap, per walked tree
  int maxWidthPct = 100; // absent in JSON = full width
  //  Per-corner radii [TL, TR, BR, BL], absent = square.
  int corners[4] = {0, 0, 0, 0};
};

//  Pure layout resolution: Theme + (card, grouped) -> ResolvedRow.
//  No QModelIndex, no QJson here; the view/delegate extracts
//  (type/own/grouped) from the model and calls into this.
class RowResolver {
public:
  //  Message type (MT_*) + own flag -> card name.
  //  "layout_" + card (+ "_next" for in-series) names the tree to walk.
  static QString roleForCard(int messageType, bool own);
  //  Single place that builds the layout_* name; in-series only for
  //  incoming/outgoing, other cards ignore the grouped flag.
  static QString layoutNameForCard(const QString& card, bool grouped);
  //  Avatar space is structural: the avatar node sits in series-start
  //  trees only, so a resolved row either carries it or not.
  static bool reservesAvatar(const QString& card, int avatarSize);
  static ResolvedRow resolve(const Theme& theme, const QString& card, bool grouped);
};
