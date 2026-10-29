// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QMap>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include "core/theme.h"
#include "gui/chat/bubblegeometry.h"
#include "gui/chat/rowresolver.h"

//  Native chat delegate: renders rows resolved by RowResolver.
//  In-series rows walk layout_incoming_next/layout_outgoing_next:
//  header and avatar live in series-start trees only.
class BubbleDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  struct DisplaySettings {
    int fontSizeVal = 1;
    bool messageTime = true;
    bool messageDate = false;
    bool allowLinks = true;
    bool pathToLink = false;
    bool trimMessage = true;
    bool autoFile = false;

    bool operator==(const DisplaySettings& other) const {
      return fontSizeVal == other.fontSizeVal &&
             messageTime == other.messageTime && messageDate == other.messageDate &&
             allowLinks == other.allowLinks && pathToLink == other.pathToLink &&
             trimMessage == other.trimMessage && autoFile == other.autoFile;
    }
    bool operator!=(const DisplaySettings& other) const {
      return !(*this == other);
    }
  };

  explicit BubbleDelegate(QObject* parent = nullptr);

  void setTheme(const Theme& theme);
  void setMode(Theme::Mode mode, bool systemDark);
  void setDisplaySettings(const DisplaySettings& settings);
  void invalidateCache();
  void noteViewportWidth(int viewportWidth);

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  //  Theme + display size only. Public for tests.
  QFont headerNameFont() const;
  QFont headerTimeFont() const;
  //  Row layout. Alias kept for tests.
  using ResolvedRow = ::ResolvedRow;
  ResolvedRow resolveRow(const QModelIndex& index) const;
  //  Geometry only, no model access.
  QRect rowBubbleRect(int viewportWidth, int contentH, const ResolvedRow& row) const;

  //  Link hit-test: href under pos or empty.
  QString anchorAt(const QPoint& pos, const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
  //  Anchor rect in view coordinates.
  QRect anchorRect(const QString& href, const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
  QStringList anchors(const QModelIndex& index, const QStyleOptionViewItem& option) const;
  //  Tab-focused anchor highlight.
  void setFocusAnchor(int row, const QString& href);
  void clearFocusAnchor();
  //  Fragment selection inside one bubble.
  int charPosAt(const QPoint& pos, const QStyleOptionViewItem& option,
                const QModelIndex& index) const;
  int charPosClamped(const QPoint& pos, const QStyleOptionViewItem& option,
                     const QModelIndex& index) const;
  void setTextSelection(int row, int start, int end);
  void extendSelection(int pos);
  void clearTextSelection();
  bool hasTextSelection() const;
  int selectionRow() const;
  QString selectedText() const;
  //  QTextDocument whitespace -> plain spaces and newlines.
  static QString normalizePlainText(QString text);
  void selectAllText(const QModelIndex& index, const QStyleOptionViewItem& option);
  //  Final row HTML. Public for tests.
  QString rowHtml(const QModelIndex& index, const ResolvedRow& row) const;

private:
  struct CacheEntry {
    QTextDocument* doc = nullptr;
    int width = -1;
    ResolvedRow row;
    RowGeometry geo;
  };

  double localPointSize() const;
  QString rowTime(const QModelIndex& index) const;
  QString fileCardHtml(const QModelIndex& index) const;
  int viewWidth(const QStyleOptionViewItem& option) const;
  //  sizeHint cache key: (row, viewportWidth, DPR, contentRev).
  QString cacheKey(const QModelIndex& index, int viewportWidth) const;
  QString contentRev(const QModelIndex& index) const;
  const CacheEntry& entryFor(const QModelIndex& index, int viewportWidth) const;
  QPixmap avatarPixmap(const QString& path, int size) const;

  Theme theme;
  Theme::Mode mode = Theme::Mode::System;
  bool systemDark = false;
  DisplaySettings display;
  int focusRow = -1;
  QString focusHref;
  int selRow = -1;
  int selStart = 0;
  int selEnd = 0;
  mutable QMap<QString, CacheEntry> cache;
  mutable QMap<QString, QPixmap> avatars;
};
