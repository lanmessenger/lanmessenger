// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QMap>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include "core/themev2.h"

//  Native chat delegate: tree hardcoded 1:1 with the future
//  layout_incoming/outgoing (S subset avatar/header/bubble/
//  body/fileCard/status; the validator accepts linkPreview/quote/code/buttons,
//  the delegate ignores them). Stage 2 reads layout_* from the same theme.json.
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

  void setTheme(const ThemeV2& theme);
  void setMode(ThemeV2::Mode mode, bool systemDark);
  void setDisplaySettings(const DisplaySettings& settings);
  void invalidateCache();

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  //  Link hit-test: href under pos or empty. Coordinate mapping as in paint.
  QString anchorAt(const QPoint& pos, const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
  //  Anchor rect in view coordinates (Tab-focus highlight).
  QRect anchorRect(const QString& href, const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
  //  All row hrefs in document order (Tab navigation).
  QStringList anchors(const QModelIndex& index, int viewportWidth) const;
  //  Tab-focused anchor highlight (view holds position, delegate paints).
  void setFocusAnchor(int row, const QString& href);
  void clearFocusAnchor();

private:
  struct RowDoc {
    QTextDocument* doc = nullptr;
    QRect bubbleRect;
    QRect docRect;
  };

  struct CacheEntry {
    QTextDocument* doc = nullptr;
    QSize size;
    QRect bubbleRect;
    QRect docRect;
  };

  QString cardFor(const QModelIndex& index) const;
  QColor cardColor(const QString& cardName, bool bg2, bool darkMode) const;
  QFont rowFont(const QModelIndex& index) const;
  QString rowTime(const QModelIndex& index) const;
  QString rowHtml(const QModelIndex& index) const;
  QString decodeBody(QString text) const;
  QString fileCardHtml(const QModelIndex& index) const;
  int contentWidth(const QModelIndex& index, int viewportWidth) const;
  QRect rowBubbleRect(const QModelIndex& index, int viewportWidth, int contentH) const;
  //  sizeHint cache key: (row, viewportWidth, DPR, theme/mode, settings,
  //  contentRev). contentRev covers text/avatar/fileop of the same row.
  QString cacheKey(const QModelIndex& index, int viewportWidth) const;
  QString contentRev(const QModelIndex& index) const;
  const CacheEntry& entryFor(const QModelIndex& index, int viewportWidth) const;
  QPixmap avatarPixmap(const QString& path, int size) const;
  //  Avatar space is reserved only when the avatar is actually painted:
  //  grouped rows hide it unless the theme opts in via showOnGrouped.
  bool reservesAvatar(const QString& card, bool grouped) const;

  ThemeV2 theme;
  ThemeV2::Mode mode = ThemeV2::Mode::System;
  bool systemDark = false;
  DisplaySettings display;
  int themeRev = 0;
  int focusRow = -1;
  QString focusHref;
  mutable QMap<QString, CacheEntry> cache;
  mutable QMap<QString, QPixmap> avatars;
};
