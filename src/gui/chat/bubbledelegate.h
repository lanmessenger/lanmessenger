// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QMap>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include "core/theme.h"

//  Native chat delegate: resolves layout_* into a row (S subset
//  avatar/header/body/fileCard; the validator accepts linkPreview/quote/
//  code/buttons, the delegate ignores them).
//  In-series rows walk layout_incoming_next/layout_outgoing_next:
//  header and avatar are structural, never flag-gated.
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
  //  Top gap of the row: a per-tree scalar (series-start trees carry
  //  the wide gap, _next trees the dense in-series one).
  //  Public for tests.
  int rowTopMargin(const QModelIndex& index) const;
  //  Per-corner radii [TL, TR, BR, BL] from the walked tree.
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
    Theme::DualColor text;        // body text: node value or theme default
    Theme::DualColor senderColor; // header name: node value or theme default
    Theme::DualColor timeColor;   // header time: node value or theme default
    Theme::DualColor border;      // bubble outline
    bool hasBorder = false;
    Theme::Bg bg; // bubble fill
    //  Rect metrics from the layout body/fileCard node. Rows are tailless.
    int padding[4] = {0, 0, 0, 0}; // [T, R, B, L]
    double shadowDx = 0.0;
    double shadowDy = 0.0;
    double shadowBlur = 0.0;
    double shadowOpacity = 0.0;
    int margin = 0; // row top gap, per walked tree
    int maxWidthPct = 75;
    //  Per-corner radii [TL, TR, BR, BL] from the layout body/fileCard node
    //  (hand-mirrored by the theme author for outgoing; no code mirror).
    int corners[4] = {12, 12, 12, 4};
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
    bool hasAvatar = false;
    bool hasHeader = false;
    QStringList headerItems;
    Theme::DualColor senderColor;
    Theme::DualColor timeColor;
    Theme::DualColor border;
    bool hasBorder = false;
    Theme::Bg bg;
    double shadowDx = 0.0;
    double shadowDy = 0.0;
    double shadowBlur = 0.0;
    double shadowOpacity = 0.0;
  };

  QString roleFor(const QModelIndex& index) const;
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
  //  Avatar space is reserved only when the avatar node is walked:
  //  series-start trees carry it, _next trees omit it.
  bool reservesAvatar(const QString& card) const;

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
