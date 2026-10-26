// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "gui/chat/anchornavigator.h"

#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>

QString AnchorNavigator::anchorAt(const QTextDocument* doc, const QRect& docRect,
                                  const QPoint& pos) {
  if(doc == nullptr || !docRect.contains(pos))
    return QString();
  const QPointF docPos = QPointF(pos - docRect.topLeft());
  return doc->documentLayout()->anchorAt(docPos);
}

QRect AnchorNavigator::anchorRect(const QTextDocument* doc, const QRect& docRect,
                                  const QString& href) {
  if(href.isEmpty() || doc == nullptr)
    return QRect();
  for(QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
    if(block.layout() == nullptr)
      continue;
    const QRect blockRect = doc->documentLayout()->blockBoundingRect(block).toRect();
    for(QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it) {
      const QTextFragment frag = it.fragment();
      if(!frag.charFormat().isAnchor() || frag.charFormat().anchorHref() != href)
        continue;
      const int start = frag.position() - block.position();
      const int end = start + frag.length();
      QTextLayout* layout = block.layout();
      for(int i = 0; i < layout->lineCount(); i++) {
        const QTextLine line = layout->lineAt(i);
        const int ls = line.textStart();
        const int le = ls + line.textLength();
        const int s = qMax(start, ls);
        const int en = qMin(end, le);
        if(s < en) {
          const qreal x1 = line.cursorToX(s);
          const qreal x2 = line.cursorToX(en);
          return QRect(docRect.left() + blockRect.left() + int(x1),
                       docRect.top() + blockRect.top() + int(line.y()), int(x2 - x1),
                       int(line.height()));
        }
      }
    }
  }
  return QRect();
}

QStringList AnchorNavigator::anchors(const QTextDocument* doc) {
  QStringList out;
  if(doc == nullptr)
    return out;
  for(QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
    for(QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it) {
      const QTextFragment frag = it.fragment();
      if(frag.charFormat().isAnchor()) {
        const QString href = frag.charFormat().anchorHref();
        if(!href.isEmpty() && !out.contains(href))
          out.append(href);
      }
    }
  }
  return out;
}
