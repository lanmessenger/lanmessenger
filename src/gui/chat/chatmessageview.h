// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QHash>
#include <QListView>

#include "core/theme.h"
#include "gui/chat/bubbledelegate.h"
#include "gui/chat/chatmessagemodel.h"
#include "messaging/xmlmessage.h"

class QMenu;
class QAction;
class QDateTime;

//  Native chat view (stage 1.5, single path): QListView + ChatMessageModel +
//  BubbleDelegate. Single view for chat/chatroom/history/settings-preview.
class ChatMessageView : public QListView {
  Q_OBJECT
  Q_PROPERTY(QString plainText READ plainText NOTIFY plainTextChanged)

public:
  explicit ChatMessageView(QWidget* parent = nullptr);

  //  lmcMessageLog compatibility (used by chatwindow).
  void initChat(bool clearLog = true);
  void reloadTheme();
  void appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName,
                        XmlMessage* pMessage, bool bReload = false);
  void updateFileMessage(FileMode mode, FileOp op, QString fileId);
  void updateUserName(QString* lpszUserId, QString* lpszUserName);
  void updateAvatar(QString* lpszUserId, QString* lpszFilePath);
  void reloadMessageLog();
  QString prepareMessageLogForSave(OutputFormat format = HtmlFormat);
  void setAutoScroll(bool enable);
  void abortPendingFileOperations();
  void saveMessageLog(QString filePath);
  void restoreMessageLog(QString filePath, bool reload = true);
  //  S1.5: legacy history (messenger.db) stores rendered HTML blobs.
  //  No migration: show the blob as one read-only body row.
  void showLegacyHtml(QString* userName, QDateTime* when, QString* html);
  void setAppearanceMode(const QString& mode);
  QString appearanceModeString() const;
  //  Theme switch by name (user dir wins over built-in qrc); unknown names
  //  are ignored, the log stays (reloadTheme repaints + invalidates cache).
  void setThemeName(const QString& name);
  QString themeNameString() const;

  QString plainText() const;
  int rowCount() const;
  QString messageAt(int row) const;
  //  OwnRole of a row (history regression: stored rows must not read as own).
  bool isOwnRow(int row) const;

  //  Fields mirror lmcMessageLog (assigned directly from chatwindow).
  QString localId;
  QString peerId;
  QString peerName;
  QHash<QString, QString> participantAvatars;
  bool hasData = false;
  int fontSizeVal = 1;
  bool showSmiley = true;
  bool autoFile = false;
  bool messageTime = true;
  bool messageDate = false;
  bool allowLinks = true;
  bool pathToLink = false;
  bool trimMessage = true;

signals:
  void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void plainTextChanged();

protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  bool viewportEvent(QEvent* event) override;
  void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
  //  Tab/Return/Copy are intercepted here, not in keyPressEvent: QListView
  //  eats Tab/Backtab before keyPressEvent (focus traversal), and the agent's
  //  posted events never reach it (trace shows event() but no keyPressEvent).
  bool event(QEvent* event) override;

private slots:
  void onRowsInserted(const QModelIndex& parent, int first, int last);
  void onPaletteChanged();
  void copyAction_triggered();
  void selectAllAction_triggered();

private:
  void syncToLayers();
  void syncModelIdentity();
  void syncBackground();
  void createContextMenu();
  void showContextMenu(const QPoint& pos);
  void activateHref(const QString& href);
  void openExternal(const QString& href);
  //  False when there are no anchors or the edge is reached (Tab then goes
  //  to the focus chain, no trap): linear scan, no wrap-around.
  bool moveAnchorFocus(int delta);
  QModelIndex anchorRowAt(const QPoint& pos, QString* href = nullptr);
  QString themeFilePath() const;

  ChatMessageModel* chatModel;
  BubbleDelegate* bubbleDelegate;
  Theme theme;
  QString themeName = QStringLiteral("ClassicSlate");
  Theme::Mode appearanceMode = Theme::Mode::System;
  bool systemDark = false;
  bool autoScroll = true;
  QMenu* contextMenu = nullptr;
  QAction* copyAction = nullptr;
  QAction* selectAllAction = nullptr;
  int focusRow = -1;
  QString focusHref;
};
