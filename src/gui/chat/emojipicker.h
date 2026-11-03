// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QStringList>

class QFont;
class QKeyEvent;
class QShowEvent;
class QPainter;
class QStyleOptionViewItem;
class QModelIndex;

//  Grid of Unicode glyphs; the delegate draws them centered, never elided.
class EmojiCellDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit EmojiCellDelegate(QObject* parent = nullptr);

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
};

class EmojiPicker : public QTableWidget {
  Q_OBJECT

public:
  //  Fixed set: no flags, chains, or skin tones.
  static const QStringList& emojis();
  static QString emojiAt(int index);
  static int columns();
  //  Widest glyph plus margin, at least 32px.
  static int cellSize(const QFont& emojiFont);

  EmojiPicker(QWidget* parent, int* selected, int actionIndex);
  ~EmojiPicker() override;

protected:
  void keyPressEvent(QKeyEvent* e) override;
  void showEvent(QShowEvent* e) override;

private:
  void pickCell(int row, int column);
  void updateCellSize();

  int* selected;
  int actionIndex;
  int max_col;
  bool picked = false;
};
