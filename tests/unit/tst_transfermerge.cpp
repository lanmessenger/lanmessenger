// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.

#include <QtTest>

#include <QDateTime>

#include "core/definitions.h"
#include "gui/files/transferwindow.h"

namespace {

FileView makeView(const QString& id, FileView::TransferMode mode, FileView::TransferState state,
                  qint64 ms) {
  FileView v(id);
  v.mode = mode;
  v.state = state;
  v.startTime = QDateTime::fromMSecsSinceEpoch(ms);
  return v;
}

} // namespace

class TstTransferMerge : public QObject {
  Q_OBJECT

private slots:
  void chronologicalOrder();
  void liveActiveWinsOverStaleStoreRow();
  void externallyClearedRowDoesNotResurrect();
  void unsavedTerminalRowKept();
};

void TstTransferMerge::chronologicalOrder() {
  // Store is ORDER BY created_at: surviving active rows slot into time position.
  QList<FileView> storeViews;
  storeViews << makeView(QStringLiteral("a"), FileView::TM_Send, FileView::TS_Complete, 1000)
             << makeView(QStringLiteral("c"), FileView::TM_Send, FileView::TS_Complete, 3000);
  QList<FileView> snapshot;
  snapshot << makeView(QStringLiteral("b"), FileView::TM_Send, FileView::TS_Send, 2000);
  QList<FileView> merged = lmcTransferWindow::mergeTransferViews(storeViews, snapshot, {});
  QCOMPARE(merged.count(), 3);
  QCOMPARE(merged.at(0).id, QStringLiteral("a"));
  QCOMPARE(merged.at(1).id, QStringLiteral("b"));
  QCOMPARE(merged.at(2).id, QStringLiteral("c"));
}

void TstTransferMerge::liveActiveWinsOverStaleStoreRow() {
  QList<FileView> storeViews;
  FileView stale =
    makeView(QStringLiteral("k"), FileView::TM_Send, FileView::TS_Complete, 1000);
  stale.fileDisplay = QStringLiteral("stale");
  storeViews << stale;
  QList<FileView> snapshot;
  FileView live = makeView(QStringLiteral("k"), FileView::TM_Send, FileView::TS_Send, 1000);
  live.fileDisplay = QStringLiteral("live");
  snapshot << live;
  QSet<QString> oldKeys;
  oldKeys << QStringLiteral("k") + QLatin1Char('\x1f') + QString::number(int(FileView::TM_Send));
  QList<FileView> merged = lmcTransferWindow::mergeTransferViews(storeViews, snapshot, oldKeys);
  QCOMPARE(merged.count(), 1);
  QCOMPARE(int(merged.first().state), int(FileView::TS_Send));
  QCOMPARE(merged.first().fileDisplay, QStringLiteral("live"));
}

void TstTransferMerge::externallyClearedRowDoesNotResurrect() {
  QList<FileView> snapshot;
  snapshot << makeView(QStringLiteral("k"), FileView::TM_Send, FileView::TS_Complete, 1000);
  QSet<QString> oldKeys;
  oldKeys << QStringLiteral("k") + QLatin1Char('\x1f') + QString::number(int(FileView::TM_Send));
  QList<FileView> merged = lmcTransferWindow::mergeTransferViews({}, snapshot, oldKeys);
  QVERIFY(merged.isEmpty());
}

void TstTransferMerge::unsavedTerminalRowKept() {
  QList<FileView> snapshot;
  snapshot << makeView(QStringLiteral("k"), FileView::TM_Receive, FileView::TS_Cancel, 1000);
  QList<FileView> merged = lmcTransferWindow::mergeTransferViews({}, snapshot, {});
  QCOMPARE(merged.count(), 1);
  QCOMPARE(merged.first().id, QStringLiteral("k"));
}

QTEST_MAIN(TstTransferMerge)
#include "tst_transfermerge.moc"
