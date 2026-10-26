// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "gui/chat/bubblegeometry.h"

#include <QFontMetrics>
#include <QtMath>

namespace {

const int kEdgeGutter = 1;
const int kMinContentW = 80;

} // namespace

int BubbleGeometry::contentWidth(const ResolvedRow& row, int viewportWidth, int avatarSize) {
  QString card = row.card;
  int avail = viewportWidth;
  if(row.avatar)
    avail -= avatarSize + row.hboxGap;
  //  Strip cards keep 1px gutters on both sides for the border stroke.
  if(card == QLatin1String("broadcast") || card == QLatin1String("system"))
    avail -= 2 * kEdgeGutter;
  if(card == QLatin1String("incoming") || card == QLatin1String("outgoing") ||
     card == QLatin1String("request"))
    avail = avail * row.maxWidthPct / 100;
  int padH = row.padding[1] + row.padding[3];
  return qMax(kMinContentW, avail - padH);
}

QRect BubbleGeometry::placeBubble(const ResolvedRow& row, int viewportWidth, int contentW,
                                  int contentH, int headerH, int avatarSize) {
  QString card = row.card;
  int bubbleW = contentW + row.padding[1] + row.padding[3];
  int bubbleH = contentH + row.padding[0] + row.padding[2];
  int x = 0;
  int top = row.marginTop;
  int avatarW = 0;
  if(row.avatar)
    avatarW = avatarSize + row.hboxGap;
  if(card == QLatin1String("outgoing")) {
    x = viewportWidth - bubbleW;
    if(x < kEdgeGutter) {
      bubbleW -= kEdgeGutter - x;
      x = kEdgeGutter;
    }
  } else if(card == QLatin1String("system")) {
    x = (viewportWidth - bubbleW) / 2;
  } else if(card == QLatin1String("broadcast")) {
    x = kEdgeGutter;
  } else {
    x = qMax(avatarW, kEdgeGutter);
  }
  return QRect(x, top + headerH, bubbleW, bubbleH);
}

RowGeometry BubbleGeometry::buildLayout(const ResolvedRow& row, int viewportWidth, int contentW,
                                        int contentH, const QFont& nameFont,
                                        const QFont& timeFont, int avatarSize) {
  RowGeometry l;
  l.contentW = contentW;
  l.contentH = contentH;
  if(row.header) {
    QFontMetrics nfm(nameFont);
    QFontMetrics tfm(timeFont);
    l.headerH = qMax(nfm.height(), tfm.height()) + row.vboxGap;
    l.headerAscent = qMax(nfm.ascent(), tfm.ascent());
  }
  l.bubbleRect = placeBubble(row, viewportWidth, contentW, contentH, l.headerH, avatarSize);
  l.docRect = QRect(l.bubbleRect.left() + row.padding[3], l.bubbleRect.top() + row.padding[0],
                    contentW, contentH);
  l.size = QSize(viewportWidth, l.bubbleRect.bottom() + 1);
  return l;
}
