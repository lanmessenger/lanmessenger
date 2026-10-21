// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QTextBrowser>

class QMessageBrowser : public QTextBrowser {
  Q_OBJECT

public:
  explicit QMessageBrowser(QWidget* parent = nullptr);
  virtual ~QMessageBrowser();

  void insertMoreMessagesAnchor(const QString& text);
  void insertMoreMessagesAnchor(QTextCursor cursor, const QString& text);
  void insertMessage(QTextCursor cursor, const QString& sender, const QString& receiver, const QDateTime& time, const QString& avatarUrl, const QString& text);

  typedef struct {
    QTextCursor cursor;
    int scrollBarMaximum;
  } InsertWithoutScrollingData;

  InsertWithoutScrollingData beginInsertWithoutScrolling();
  void endInsertWithoutScrollig(InsertWithoutScrollingData data);

private slots:
  void onAnchorClicked(const QUrl& arg1);

Q_SIGNALS:
  void moreMessagesAnchorClicked();
};
