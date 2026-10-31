// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
//
// LEGACY: structured-history blob parser (data-lmc-history="2").
// Extracted from: gui/chat/chatmessageview.cpp (parseStructuredHistory).
// Replaced by: src/storage/historystore.* row storage (no HTML blobs).
// Delete after: the SQLite release + one successful legacy import cycle.
// New code must not use this except the one-shot import in
// HistoryStore::open.
#include "historyparser.h"

#include <QRegularExpression>
#include <QTextDocument>

#include "core/definitions.h"
#include "gui/chat/bubbledelegate.h"

namespace {

QString historyAttr(const QString& attrs, const char* name) {
  QRegularExpression re(QStringLiteral("\\b") + QLatin1String(name) +
                        QStringLiteral("=\"([^\"]*)\""));
  QRegularExpressionMatch m = re.match(attrs);
  return m.hasMatch() ? m.captured(1) : QString();
}

QString historySpan(const QString& inner, const char* cls) {
  QRegularExpression openRe(
    QStringLiteral("<span class=['\"]") + QLatin1String(cls) + QStringLiteral("['\"]>"),
    QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatch open = openRe.match(inner);
  if(!open.hasMatch())
    return QString();
  //  Balanced close: S1 emoji spans nest inside the message span, so the
  //  first </span> is not necessarily ours. Old blobs have no nesting
  //  and behave exactly as before.
  int depth = 1;
  int pos = open.capturedEnd();
  int contentEnd = -1;
  while(depth > 0 && pos < inner.length()) {
    int nested = inner.indexOf(QStringLiteral("<span"), pos, Qt::CaseInsensitive);
    int close = inner.indexOf(QStringLiteral("</span>"), pos, Qt::CaseInsensitive);
    if(close == -1)
      return QString();
    if(nested != -1 && nested < close) {
      int tagEnd = inner.indexOf(QLatin1Char('>'), nested);
      if(tagEnd == -1)
        return QString();
      //  A self-closed <span/> carries no content: no depth change.
      if(inner.at(tagEnd - 1) != QLatin1Char('/'))
        depth++;
      pos = tagEnd + 1;
    } else {
      depth--;
      if(depth == 0)
        contentEnd = close;
      pos = close + 7;
    }
  }
  if(contentEnd == -1)
    return QString();
  QTextDocument doc;
  doc.setHtml(inner.mid(open.capturedEnd(), contentEnd - open.capturedEnd()));
  return BubbleDelegate::normalizePlainText(doc.toPlainText());
}

} // namespace

//  Envelope "2": one <p> per message; false when legacy or empty.
bool parseStructuredHistory(const QString& html, QList<HistoryRecord>* out) {
  if(!html.contains(QStringLiteral("data-lmc-history=\"2\"")))
    return false;
  static const QRegularExpression pRe(
    QStringLiteral("<p\\b([^>]*)>(.*?)</p>"),
    QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatchIterator it = pRe.globalMatch(html);
  while(it.hasNext()) {
    QRegularExpressionMatch pm = it.next();
    HistoryRecord rec;
    rec.own = historyAttr(pm.captured(1), "data-own") == QLatin1String("1");
    rec.ts = historyAttr(pm.captured(1), "data-ts").toLongLong();
    rec.type = historyAttr(pm.captured(1), "data-type").toInt() == int(MT_GroupMessage)
                 ? int(MT_GroupMessage)
                 : int(MT_Message);
    QString name = historySpan(pm.captured(2), "salutation");
    if(name.endsWith(QLatin1Char(':')))
      name.chop(1);
    rec.userName = name;
    QString body = historySpan(pm.captured(2), "message");
    while(body.endsWith(QLatin1Char('\n')))
      body.chop(1);
    rec.body = body;
    if(rec.body.isEmpty() || rec.userName.isEmpty())
      continue;
    out->append(rec);
  }
  return !out->isEmpty();
}

bool isStructuredHistoryBlob(const QString& html) {
  return html.contains(QStringLiteral("data-lmc-history=\"2\""));
}

QString legacyHtmlToPlain(const QString& html) {
  QTextDocument doc;
  doc.setHtml(html);
  return BubbleDelegate::normalizePlainText(doc.toPlainText()).trimmed();
}
