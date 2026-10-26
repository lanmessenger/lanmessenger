// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QFont>
#include <QRect>
#include <QSize>

#include "gui/chat/rowresolver.h"

//  Single geometry result for one row: content size, header metrics,
//  bubble/doc rects (unshifted, view adds option.rect.top) and row size.
struct RowGeometry {
  int contentW = 0;
  int contentH = 0;
  int headerH = 0;
  int headerAscent = 0;
  QRect bubbleRect;
  QRect docRect;
  QSize size;
};

//  Pure bubble geometry: ResolvedRow + viewport + fonts -> RowGeometry.
//  No model, no QTextDocument, no cache here.
class BubbleGeometry {
public:
  static int contentWidth(const ResolvedRow& row, int viewportWidth, int avatarSize);
  static QRect placeBubble(int viewportWidth, int contentW, int contentH,
                           const ResolvedRow& row, int headerH, int avatarSize);
  static RowGeometry buildLayout(const ResolvedRow& row, int viewportWidth, int contentW,
                                 int contentH, const QFont& nameFont, const QFont& timeFont,
                                 int avatarSize);
};
