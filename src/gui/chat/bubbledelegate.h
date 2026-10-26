// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QMap>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include "core/theme.h"
#include "gui/chat/rowresolver.h"

//  Native chat delegate: renders rows resolved by RowResolver (S subset
//  avatar/header/body/fileCard; the validator accepts linkPreview/quote/
//  code/buttons, the delegate ignores them).
//  In-series rows walk layout_incoming_next/layout_outgoing_next:
//  header and avatar live in series-start trees only.
class BubbleDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  struct DisplaySettings {
    bool showSmiley = true;
    int fontSizeVal = 1;
    bool messageTime = true;
    bool messageDate = false;
    bool allowLinks = true;
    bool pathToLink = false;
    bool trimMessage = true;
    bool autoFile = false;

    bool operator==(const DisplaySettings& other) const {
      return showSmiley == other.showSmiley && fontSizeVal == other.fontSizeVal &&
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
  //  Theme + local display size only (never the message font). Public for tests.
  QFont headerNameFont() const;
  QFont headerTimeFont() const;
  //  Layout resolution for one row: RowResolver walks the compiled
  //  layout_* once per row (inside entryFor), never touching QJson.
  //  Alias kept so tests use BubbleDelegate::ResolvedRow as before.
  using ResolvedRow = ::ResolvedRow;
  ResolvedRow resolveRow(const QModelIndex& index) const;
  //  Public for tests.
  QRect rowBubbleRect(const QModelIndex& index, int viewportWidth, int contentH,
                      const ResolvedRow& row) const;

  //  Link hit-test: href under pos or empty. Coordinate mapping as in paint.
  QString anchorAt(const QPoint& pos, const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
  //  Anchor rect in view coordinates (Tab-focus highlight).
  QRect anchorRect(const QString& href, const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
  QStringList anchors(const QModelIndex& index, const QStyleOptionViewItem& option) const;
  //  Tab-focused anchor highlight (view holds position, delegate paints).
  void setFocusAnchor(int row, const QString& href);
  void clearFocusAnchor();
  //  Final row HTML (font/color guard + body). Public for tests.
  QString rowHtml(const QModelIndex& index, const ResolvedRow& row) const;

private:
  struct RowDoc {
    QTextDocument* doc = nullptr;
    QRect bubbleRect;
    QRect docRect;
  };

  struct CacheEntry {
    QTextDocument* doc = nullptr;
    int width = -1;
    ResolvedRow row;
    int headerH = 0;
    int headerAscent = 0;
    QSize size;
    QRect bubbleRect;
    QRect docRect;
  };

  struct HeaderMetrics {
    int height = 0;
    int ascent = 0;
  };
  HeaderMetrics headerMetrics(const ResolvedRow& row) const;
  struct RowLayout {
    int contentW = 0;
    int contentH = 0;
    int headerH = 0;
    int headerAscent = 0;
    QRect bubbleRect;
    QRect docRect;
    QSize size;
  };
  RowLayout buildLayout(int viewportWidth, const ResolvedRow& row, int contentW,
                        int contentH) const;
  QRect placeBubble(int viewportWidth, int contentW, int contentH, const ResolvedRow& row,
                    int headerH) const;

  QString roleFor(const QModelIndex& index) const;
  double localPointSize() const;
  QFont rowFont(const QModelIndex& index) const;
  QString rowTime(const QModelIndex& index) const;
  QString decodeBody(QString text) const;
  QString fileCardHtml(const QModelIndex& index) const;
  int contentWidth(const QModelIndex& index, int viewportWidth, const ResolvedRow& row) const;
  int viewWidth(const QStyleOptionViewItem& option) const;
  //  sizeHint cache key: (row, viewportWidth, DPR, contentRev).
  QString cacheKey(const QModelIndex& index, int viewportWidth) const;
  QString contentRev(const QModelIndex& index) const;
  const CacheEntry& entryFor(const QModelIndex& index, int viewportWidth) const;
  QPixmap avatarPixmap(const QString& path, int size) const;
  //  Avatar space is reserved only when the avatar node is walked:
  //  series-start trees carry it, _next trees omit it.
  bool reservesAvatar(const QString& card) const;

  Theme theme;
  Theme::Mode mode = Theme::Mode::System;
  bool systemDark = false;
  DisplaySettings display;
  int focusRow = -1;
  QString focusHref;
  mutable QMap<QString, CacheEntry> cache;
  mutable QMap<QString, QPixmap> avatars;
};
