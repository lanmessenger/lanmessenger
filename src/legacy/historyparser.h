// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
//
// LEGACY: structured-history blob parser (data-lmc-history="2").
// Extracted from: gui/chat/chatmessageview.cpp (parseStructuredHistory).
// Replaced by: src/storage/historystore.* row storage (no HTML blobs).
// Delete after: the SQLite release + one successful legacy import cycle.
// New code must not use this except the one-shot import in
// HistoryStore::open.
#pragma once

#include <QList>
#include <QString>

struct HistoryRecord {
  QString userName;
  bool own = false;
  qint64 ts = 0;
  int type = 0;
  QString body;
};

bool parseStructuredHistory(const QString& html, QList<HistoryRecord>* out);
bool isStructuredHistoryBlob(const QString& html);
QString legacyHtmlToPlain(const QString& html);
