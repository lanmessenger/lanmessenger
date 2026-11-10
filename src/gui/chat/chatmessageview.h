// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QListView>

#include "core/theme.h"
#include "gui/chat/bubbledelegate.h"
#include "gui/chat/chatmessagemodel.h"
#include "messaging/xmlmessage.h"

struct StoredMessage;

class QMenu;
class QAction;
class QDateTime;
class QTimer;
class QFocusEvent;
class QHideEvent;

//  Native chat view: QListView + ChatMessageModel + BubbleDelegate.
//  Single view for chat/chatroom/history/settings-preview.
class ChatMessageView : public QListView {
  Q_OBJECT
  Q_PROPERTY(QString plainText READ plainText NOTIFY plainTextChanged)

public:
  explicit ChatMessageView(QWidget* parent = nullptr);

  //  lmcMessageLog compatibility.
  void initChat(bool clearLog = true);
  void reloadTheme();
  void syncBackground();
  void appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName,
                        XmlMessage* pMessage, bool bReload = false);
  void updateFileMessage(FileMode mode, FileOp op, QString fileId);
  void updateUserName(QString* lpszUserId, QString* lpszUserName);
  void updateAvatar(QString* lpszUserId, QString* lpszFilePath);
  void reloadMessageLog();
  QString prepareMessageLogForSave(OutputFormat format = HtmlFormat);
  QList<StoredMessage> storedMessages() const;
  void setAutoScroll(bool enable);
  void abortPendingFileOperations();
  void saveMessageLog(QString filePath);
  bool restoreMessageLog(QString filePath, bool reload = true);
  //  Legacy history blob: one read-only row.
  void showLegacyHtml(QString* userName, QDateTime* when, QString* html);
  static bool isStructuredHistory(const QString* html);
  static QString historyLocalId();
  void showHistoryMessages(QString* userName, QDateTime* when, QString* html);
  void setAppearanceMode(const QString& mode);
  QString appearanceModeString() const;
  void setThemeName(const QString& name);
  QString themeNameString() const;

  QString plainText() const;
  int rowCount() const;
  QString messageAt(int row) const;
  bool isOwnRow(int row) const;

  //  Fields mirror lmcMessageLog.
  QString localId;
  QString peerId;
  QString peerName;
  QHash<QString, QString> participantAvatars;
  bool hasData = false;
  int fontSizeVal = 1;
  bool autoFile = false;
  bool allowLinks = true;
  bool pathToLink = false;
  bool trimMessage = true;

signals:
  void messageSent(MessageType type, QString* lpszUserId, XmlMessage* pMessage);
  void plainTextChanged();

protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void focusOutEvent(QFocusEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  bool viewportEvent(QEvent* event) override;
  void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
  bool event(QEvent* event) override;

private slots:
  void onRowsInserted(const QModelIndex& parent, int first, int last);
  void onRowsRemoved(const QModelIndex& parent, int first, int last);
  void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                     const QList<int>& roles);
  void onModelReset();
  void onPaletteChanged();
  void copyAction_triggered();
  void selectAllAction_triggered();
  void onSelectionScrollTick();

private:
  void extendSelectionTo(const QPoint& pos);
  void startOrUpdateSelectionScroll(const QPoint& pos);
  void stopSelectionScroll();
  void finishTextDrag();
  void setCurrentWithoutScroll(const QModelIndex& index);
  void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;
  void syncToLayers();
  void syncModelIdentity();
  void requestRelayout();
  void createContextMenu();
  void showContextMenu(const QPoint& pos);
  void activateHref(const QString& href);
  void openExternal(const QString& href);
  bool moveAnchorFocus(int delta);
  QModelIndex anchorRowAt(const QPoint& pos, QString* href = nullptr);
  QStyleOptionViewItem indexOption(const QModelIndex& idx) const;
  QModelIndex textIndexAt(const QPoint& pos, int* charPos = nullptr) const;
  bool tryTripleSelect(const QModelIndex& textIdx);
  void resetWordSelect();
  void clearTextSelection();
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
  bool textSelecting = false;
  bool wordSelecting = false;
  QElapsedTimer wordSelectTimer;
  int wordSelectRow = -1;
  int contextRow = -1;
  //  Viewport moves only through explicit paths (keyboard navigation,
  //  auto-follow, anchor focus). EnsureVisible fired on our behalf — presses,
  //  menus, focus transitions, layout replays, deferred Qt timers — must not
  //  scroll: a press always lands on visible content.
  bool allowScrollTo = false;
  //  RAII gate for allowScrollTo: restores the previous value on scope exit,
  //  so nested uses cannot clear an outer allowance early.
  class ScrollToGuard {
  public:
    explicit ScrollToGuard(bool* flag)
        : m_flag(flag), m_prev(*flag) {
      *m_flag = true;
    }
    ~ScrollToGuard() {
      *m_flag = m_prev;
    }
    ScrollToGuard(const ScrollToGuard&) = delete;
    ScrollToGuard& operator=(const ScrollToGuard&) = delete;

  private:
    bool* m_flag;
    bool m_prev;
  };
  QTimer* selectionScrollTimer = nullptr;
  QPoint selectionScrollPos;
  int selectionScrollDir = 0;
};
