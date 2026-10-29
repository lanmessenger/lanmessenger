// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QTableWidget>
#include <QStringList>

class QModelIndex;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QShowEvent;
class QEvent;

//  Emoji picker: grid of Unicode glyphs rendered by the OS font.
class EmojiPicker : public QTableWidget {
  Q_OBJECT

public:
  //  Fixed set: no flags, chains, or skin tones.
  static const QStringList& emojis();
  static QString emojiAt(int index);
  static int columns();

  EmojiPicker(QWidget* parent, int* selected, int actionIndex);
  ~EmojiPicker() override;

protected:
  void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
  void keyPressEvent(QKeyEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void paintEvent(QPaintEvent* e) override;
  void showEvent(QShowEvent* e) override;
  void leaveEvent(QEvent* e) override;

private:
  void pickCell(int row, int column);

  int* selected;
  int actionIndex;
  int max_col;
  bool picked = false;

  QTableWidgetItem* hoverItem;
};
