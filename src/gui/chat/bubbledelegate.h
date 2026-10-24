// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QMap>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include "core/theme.h"

//  Native chat delegate: resolves layout_* into a row (S subset
//  avatar/header/bubble/body/fileCard/status; the validator accepts
//  linkPreview/quote/buttons, the delegate ignores them).
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

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  //  Theme + local display size only (never the message font). Public for tests.
  QFont headerNameFont() const;
  QFont headerTimeFont() const;
  //  Top gap of the row: series start (or single) vs inside a series.
  //  Public for tests.
  int rowTopMargin(const QModelIndex& index) const;
  //  Per-corner radii [TL, TR, BR, BL] resolved the same way.
  //  Public for tests.
  void rowCornerRadii(const QModelIndex& index, int out[4]) const;
  //  Layout resolution for one row: compiled layout_* walked once per row
  //  (inside entryFor), never touching QJson. Public for tests.
  struct ResolvedRow {
    QString card;
    bool avatar = false;
    bool header = false;
    QStringList headerItems;
    int hboxGap = 8;
    int vboxGap = 2;
    QString bubbleTextColor; // $token ref or empty = card default
    QString senderColor;     // $token ref or empty = theme default
    QString timeColor;       // $token ref or empty = theme default
  };
  ResolvedRow resolveRow(const QModelIndex& index) const;

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
    int topMargin = 0;
    int corner[4] = {0, 0, 0, 0};
    bool hasHeader = false;
    QStringList headerItems;
    QString senderColorRef;
    QString timeColorRef;
    QString bubbleTextRef;
  };

  QString cardFor(const QModelIndex& index) const;
  QColor cardColor(const QString& cardName, bool bg2, bool darkMode) const;
  double localPointSize() const;
  QFont rowFont(const QModelIndex& index) const;
  QString rowTime(const QModelIndex& index) const;
  QString rowHtml(const QModelIndex& index, const ResolvedRow& row) const;
  QString decodeBody(QString text) const;
  QString fileCardHtml(const QModelIndex& index) const;
  int contentWidth(const QModelIndex& index, int viewportWidth, const ResolvedRow& row) const;
  QRect rowBubbleRect(const QModelIndex& index, int viewportWidth, int contentH,
                      const ResolvedRow& row) const;
  //  sizeHint cache key: (row, viewportWidth, DPR, theme/mode, settings,
  //  contentRev). contentRev covers text/avatar/fileop of the same row.
  QString cacheKey(const QModelIndex& index, int viewportWidth) const;
  QString contentRev(const QModelIndex& index) const;
  const CacheEntry& entryFor(const QModelIndex& index, int viewportWidth) const;
  QPixmap avatarPixmap(const QString& path, int size) const;
  //  Avatar space is reserved only when the avatar is actually painted:
  //  grouped rows hide it unless the theme opts in via showOnGrouped.
  bool reservesAvatar(const QString& card, bool grouped) const;

  Theme theme;
  Theme::Mode mode = Theme::Mode::System;
  bool systemDark = false;
  DisplaySettings display;
  int themeRev = 0;
  int focusRow = -1;
  QString focusHref;
  mutable QMap<QString, CacheEntry> cache;
  mutable QMap<QString, QPixmap> avatars;
};
