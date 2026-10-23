// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "chatmessageview.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QScrollBar>
#include <QToolTip>
#include <QUrl>

#include "core/shared.h"
#include "core/stdlocation.h"
#include "core/lmccategories.h"
#include "gui/widgets/actionhelper.h"

namespace {

//  decodeMessage with useDefaults=true (for file save): links are force-detected,
//  smileys stay as text, no trim.
void decodeForSave(QString* text) {
  text->replace(QRegularExpression(QStringLiteral("((?:(?:https?|ftp|file)://|www\\.|ftp\\.)"
                                                  "[-A-Z0-9+&@#/%=~_|$?!:,.]*[A-Z0-9+&@#/%=~_|$])"),
                                   QRegularExpression::CaseInsensitiveOption),
                QStringLiteral("<a data-isLink='true' href='\\1'>\\1</a>"));
  text->replace(QStringLiteral("<a data-isLink='true' href='www"),
                QStringLiteral("<a data-isLink='true' href='http://www"));
  QString out;
  int pos = 0;
  while(pos < text->length()) {
    int start = text->indexOf(QStringLiteral("<a data-isLink='true'"), pos);
    if(start != -1) {
      QString segment = text->mid(pos, start - pos);
      ChatHelper::makeHtmlSafe(&segment);
      out += segment;
      int end = text->indexOf(QStringLiteral("</a>"), start) + 4;
      out += text->mid(start, end - start);
      pos = end;
    } else {
      QString segment = text->mid(pos);
      ChatHelper::makeHtmlSafe(&segment);
      out += segment;
      break;
    }
  }
  out.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
  *text = out;
}

} // namespace

ChatMessageView::ChatMessageView(QWidget* parent)
    : QListView(parent) {
  setObjectName(QStringLiteral("lmcMessageLog"));
  chatModel = new ChatMessageModel(this);
  bubbleDelegate = new BubbleDelegate(this);
  setModel(chatModel);
  setItemDelegate(bubbleDelegate);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  setUniformItemSizes(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  viewport()->setMouseTracking(true);
  setAcceptDrops(false);

  createContextMenu();
  connect(chatModel, &QAbstractItemModel::rowsInserted, this, &ChatMessageView::onRowsInserted);
  connect(chatModel, &QAbstractItemModel::modelReset, this,
          &ChatMessageView::plainTextChanged);
  connect(chatModel, &QAbstractItemModel::dataChanged, this,
          &ChatMessageView::plainTextChanged);
  systemDark = qApp->palette().window().color().lightness() < 128;
}

void ChatMessageView::initMessageLog(QString newThemePath, bool clearLog) {
  //  Legacy themePath (Appearance/Theme) is no longer read; BubbleV2 is fixed.
  themePath = newThemePath;
  initChat(clearLog);
}

void ChatMessageView::initChat(bool clearLog) {
  syncToLayers();
  if(clearLog)
    chatModel->clearLog();
  hasData = chatModel->hasData;
  reloadTheme();
}

void ChatMessageView::reloadTheme() {
  theme = ThemeV2::loadFromFile(themeFilePath());
  if(!theme.isValid)
    theme = ThemeV2::builtInDefault();
  bubbleDelegate->setTheme(theme);
  bubbleDelegate->setMode(appearanceMode, systemDark);
  syncBackground();
  viewport()->update();
}

void ChatMessageView::appendMessageLog(MessageType type, QString* lpszUserId,
                                       QString* lpszUserName, XmlMessage* pMessage,
                                       bool bReload) {
  syncToLayers();
  int row = chatModel->appendMessageLog(type, lpszUserId, lpszUserName, pMessage, bReload);
  hasData = chatModel->hasData;
  //  autoFile: as messagelog getFileMessageText — accept immediately, no click.
  if(row >= 0 && !bReload && (type == MT_File || type == MT_Folder) && pMessage) {
    QString modeName = pMessage->data(XN_MODE);
    QString opName = pMessage->data(XN_FILEOP);
    if(modeName == QLatin1String("receive") && opName == QLatin1String("request") &&
       autoFile) {
      QString fileId = pMessage->data(XN_FILEID);
      QString fileType = (type == MT_Folder) ? QStringLiteral("folder") : QStringLiteral("file");
      XmlMessage out;
      if(chatModel->requestFileAction(fileId, QStringLiteral("accept"), fileType, FM_Receive,
                                      &out)) {
        emit messageSent(type, &peerId, &out);
      }
    }
  }
  Q_UNUSED(row);
}

void ChatMessageView::updateFileMessage(FileMode mode, FileOp op, QString fileId) {
  syncToLayers();
  chatModel->updateFileMessage(mode, op, fileId);
  hasData = chatModel->hasData;
}

void ChatMessageView::updateUserName(QString* lpszUserId, QString* lpszUserName) {
  syncToLayers();
  chatModel->updateUserName(lpszUserId, lpszUserName);
  viewport()->update();
}

void ChatMessageView::updateAvatar(QString* lpszUserId, QString* lpszFilePath) {
  syncToLayers();
  chatModel->updateAvatar(lpszUserId, lpszFilePath);
  viewport()->update();
}

void ChatMessageView::reloadMessageLog() {
  //  Re-read tokens + sizeHint invalidation, without clearing the log.
  syncToLayers();
  bubbleDelegate->invalidateCache();
  viewport()->update();
}

QString ChatMessageView::prepareMessageLogForSave(OutputFormat format) {
  const QList<SingleMessage> messages = chatModel->messages();
  if(format == HtmlFormat) {
    QString html =
      QStringLiteral("<html><head><style type='text/css'>"
                     "*{font-size: 9pt;} body {-webkit-nbsp-mode: space; word-wrap: break-word;}"
                     "span.salutation {float:left; font-weight: bold;} span.time {float: right;}"
                     "span.message {clear: both; display: block;} p {border-bottom: 1px solid #CCC;}"
                     "</style></head><body>");
    for(const SingleMessage& msg : messages) {
      if(msg.type != MT_Message && msg.type != MT_GroupMessage)
        continue;
      XmlMessage xml = msg.message;
      QDateTime time;
      time.setMSecsSinceEpoch(xml.header(XN_TIME).toLongLong());
      QString messageText = xml.data(XN_MESSAGE);
      if(messageText.isEmpty())
        messageText = xml.data(XN_GROUPMESSAGE);
      decodeForSave(&messageText);
      html += QStringLiteral("<p><span class='salutation'>%1:</span>"
                             "<span class='time'>%2</span>"
                             "<span class='message'>%3</span></p>")
                .arg(msg.userName.toHtmlEscaped(),
                     QLocale::system().toString(time.time(), QLocale::ShortFormat),
                     messageText);
    }
    html += QStringLiteral("</body></html>");
    return html;
  }
  QString text;
  for(const SingleMessage& msg : messages) {
    if(msg.type != MT_Message && msg.type != MT_GroupMessage)
      continue;
    XmlMessage xml = msg.message;
    QDateTime time;
    time.setMSecsSinceEpoch(xml.header(XN_TIME).toLongLong());
    QString body = xml.data(XN_MESSAGE);
    if(body.isEmpty())
      body = xml.data(XN_GROUPMESSAGE);
    text += msg.userName + QStringLiteral(" [") +
            QLocale::system().toString(time.time(), QLocale::ShortFormat) +
            QStringLiteral("]:\n") + body + QStringLiteral("\n\n");
  }
  return text;
}

void ChatMessageView::setAutoScroll(bool enable) {
  autoScroll = enable;
}

void ChatMessageView::abortPendingFileOperations() {
  chatModel->abortPendingFileOperations();
}

void ChatMessageView::saveMessageLog(QString filePath) {
  const QList<SingleMessage> messages = chatModel->messages();
  if(messages.isEmpty())
    return;
  QDir dir = QFileInfo(filePath).dir();
  if(!dir.exists())
    dir.mkpath(dir.absolutePath());
  QFile file(filePath);
  if(!file.open(QIODevice::WriteOnly))
    return;
  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);
  stream << peerId << peerName << messages;
}

void ChatMessageView::restoreMessageLog(QString filePath, bool reload) {
  QFile file(filePath);
  if(!file.open(QIODevice::ReadOnly))
    return;
  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);
  QList<SingleMessage> messages;
  stream >> peerId >> peerName >> messages;
  chatModel->setMessages(messages);
  hasData = chatModel->hasData;
  if(reload)
    reloadMessageLog();
  if(autoScroll)
    scrollToBottom();
}

void ChatMessageView::setAppearanceMode(const QString& newMode) {
  QString lower = newMode.toLower();
  if(lower == QLatin1String("light"))
    appearanceMode = ThemeV2::Mode::Light;
  else if(lower == QLatin1String("dark"))
    appearanceMode = ThemeV2::Mode::Dark;
  else
    appearanceMode = ThemeV2::Mode::System;
  bubbleDelegate->setMode(appearanceMode, systemDark);
  syncBackground();
  viewport()->update();
}

QString ChatMessageView::plainText() const {
  return chatModel->plainText();
}

int ChatMessageView::rowCount() const {
  return chatModel->rowCount();
}

QString ChatMessageView::messageAt(int row) const {
  if(row < 0 || row >= chatModel->rowCount())
    return QString();
  return chatModel->messageText(chatModel->messageAt(row));
}

void ChatMessageView::syncToLayers() {
  syncModelIdentity();
  BubbleDelegate::DisplaySettings ds;
  ds.showSmiley = showSmiley;
  ds.fontSizeVal = fontSizeVal;
  ds.messageTime = messageTime;
  ds.messageDate = messageDate;
  ds.allowLinks = allowLinks;
  ds.pathToLink = pathToLink;
  ds.trimMessage = trimMessage;
  ds.autoFile = autoFile;
  bubbleDelegate->setDisplaySettings(ds);
}

void ChatMessageView::syncModelIdentity() {
  chatModel->localId = localId;
  chatModel->participantAvatars = participantAvatars;
}

void ChatMessageView::createContextMenu() {
  contextMenu = new QMenu(this);
  copyAction = ActionHelper::createAction(contextMenu, tr("&Copy"), this,
                                          &ChatMessageView::copyAction_triggered, QIcon(),
                                          QKeySequence::Copy);
  contextMenu->addSeparator();
  selectAllAction = ActionHelper::createAction(contextMenu, tr("Select &All"), this,
                                               &ChatMessageView::selectAllAction_triggered,
                                               QIcon(), QKeySequence::SelectAll);
  connect(this, &QWidget::customContextMenuRequested, this, &ChatMessageView::showContextMenu);
  setContextMenuPolicy(Qt::CustomContextMenu);
}

void ChatMessageView::showContextMenu(const QPoint& pos) {
  bool hasSelection = !selectionModel()->selectedIndexes().isEmpty();
  copyAction->setEnabled(hasSelection);
  selectAllAction->setEnabled(chatModel->rowCount() > 0);
  contextMenu->exec(mapToGlobal(pos));
}

void ChatMessageView::copyAction_triggered() {
  QModelIndexList selected = selectionModel()->selectedIndexes();
  if(selected.isEmpty())
    return;
  std::sort(selected.begin(), selected.end(),
            [](const QModelIndex& a, const QModelIndex& b) { return a.row() < b.row(); });
  QStringList parts;
  for(const QModelIndex& idx : selected)
    parts.append(chatModel->messageText(chatModel->messageAt(idx.row())));
  QApplication::clipboard()->setText(parts.join(QStringLiteral("\n")));
}

void ChatMessageView::selectAllAction_triggered() {
  selectAll();
}

QModelIndex ChatMessageView::anchorRowAt(const QPoint& pos, QString* href) {
  QModelIndex idx = indexAt(pos);
  if(!idx.isValid())
    return QModelIndex();
  QStyleOptionViewItem opt;
  opt.initFrom(this);
  opt.rect = visualRect(idx);
  opt.widget = viewport();
  QString found = bubbleDelegate->anchorAt(viewport()->mapFromParent(pos), opt, idx);
  if(found.isEmpty())
    return QModelIndex();
  if(href)
    *href = found;
  return idx;
}

void ChatMessageView::mousePressEvent(QMouseEvent* event) {
  QString href;
  QModelIndex idx = anchorRowAt(event->pos(), &href);
  if(idx.isValid() && !href.isEmpty() && event->button() == Qt::LeftButton) {
    setCurrentIndex(idx);
    activateHref(href);
    return;
  }
  bubbleDelegate->clearFocusAnchor();
  focusRow = -1;
  focusHref.clear();
  QListView::mousePressEvent(event);
}

void ChatMessageView::mouseMoveEvent(QMouseEvent* event) {
  QString href;
  QModelIndex idx = anchorRowAt(event->pos(), &href);
  if(idx.isValid() && !href.isEmpty()) {
    viewport()->setCursor(Qt::PointingHandCursor);
    QString tip = href;
    if(href.contains(QLatin1String("/accept/")))
      tip = tr("Accept");
    else if(href.contains(QLatin1String("/decline/")))
      tip = tr("Decline");
    else if(href.contains(QLatin1String("/cancel/")))
      tip = tr("Cancel");
    QToolTip::showText(mapToGlobal(event->pos()), tip, this);
  } else {
    viewport()->unsetCursor();
    QToolTip::hideText();
  }
  QListView::mouseMoveEvent(event);
}

bool ChatMessageView::viewportEvent(QEvent* event) {
  if(event->type() == QEvent::ToolTip) {
    auto* help = static_cast<QHelpEvent*>(event);
    QString href;
    if(anchorRowAt(help->pos(), &href).isValid() && !href.isEmpty()) {
      QToolTip::showText(help->globalPos(), href, this);
      return true;
    }
  }
  return QListView::viewportEvent(event);
}

void ChatMessageView::currentChanged(const QModelIndex& current, const QModelIndex& previous) {
  QListView::currentChanged(current, previous);
  Q_UNUSED(previous);
  if(!current.isValid()) {
    bubbleDelegate->clearFocusAnchor();
    focusRow = -1;
    focusHref.clear();
  }
}

bool ChatMessageView::moveAnchorFocus(int delta) {
  int rows = chatModel->rowCount();
  if(rows == 0)
    return false;
  int width = viewport()->width();
  if(focusRow < 0) {
    int start = (delta > 0) ? 0 : rows - 1;
    int step = (delta > 0) ? 1 : -1;
    for(int r = start; r >= 0 && r < rows; r += step) {
      QStringList names = bubbleDelegate->anchors(chatModel->index(r, 0), width);
      if(!names.isEmpty()) {
        focusRow = r;
        focusHref = (delta > 0) ? names.first() : names.last();
        break;
      }
    }
  } else {
    QStringList current = bubbleDelegate->anchors(chatModel->index(focusRow, 0), width);
    int pos = current.indexOf(focusHref);
    if(pos + delta >= 0 && pos + delta < current.size()) {
      focusHref = current.at(pos + delta);
    } else {
      //  Linear scan to the list edge, no wrap-around: past the last anchor
      //  Tab must leave the view for the input/Send button (a11y, no trap).
      int r = focusRow + delta;
      bool found = false;
      while(r >= 0 && r < rows) {
        QStringList names = bubbleDelegate->anchors(chatModel->index(r, 0), width);
        if(!names.isEmpty()) {
          focusRow = r;
          focusHref = (delta > 0) ? names.first() : names.last();
          found = true;
          break;
        }
        r += delta;
      }
      if(!found) {
        focusRow = -1;
        focusHref.clear();
        bubbleDelegate->clearFocusAnchor();
        viewport()->update();
        return false;
      }
    }
  }
  bubbleDelegate->setFocusAnchor(focusRow, focusHref);
  if(focusRow >= 0)
    scrollTo(chatModel->index(focusRow, 0));
  viewport()->update();
  return focusRow >= 0 && !focusHref.isEmpty();
}

void ChatMessageView::activateHref(const QString& href) {
  if(href.startsWith(QLatin1String("lmc:"))) {
    //  lmc://file|folder/accept|decline|cancel/fileId — as onAnchorClicked.
    QStringList parts = href.split(QStringLiteral("/"), Qt::SkipEmptyParts);
    if(parts.size() < 4)
      return;
    QString fileType = parts.at(1);
    QString action = parts.at(2);
    QString fileId = parts.at(3);
    //  Cancel links exist on sender rows only (delegate shows Cancel for send
    //  mode); try the send map first as legacy onAnchorClicked, fall back to
    //  the receive map for a receiver-side cancel.
    FileMode mode = (action == QLatin1String("cancel")) ? FM_Send : FM_Receive;
    XmlMessage out;
    if(chatModel->requestFileAction(fileId, action, fileType, mode, &out)) {
      MessageType type =
        (fileType == QLatin1String("folder")) ? MT_Folder : MT_File;
      emit messageSent(type, &peerId, &out);
    } else if(action == QLatin1String("cancel")) {
      if(chatModel->requestFileAction(fileId, action, fileType, FM_Receive, &out)) {
        MessageType type =
          (fileType == QLatin1String("folder")) ? MT_Folder : MT_File;
        emit messageSent(type, &peerId, &out);
      }
    }
    focusHref.clear();
    return;
  }
  openExternal(href);
}

void ChatMessageView::openExternal(const QString& href) {
  //  Port of lmcMessageLog::onAnchorClicked for external links.
  QString link = href;
  if(link.startsWith(QLatin1String("file"))) {
    link = link.mid(5);
    QDesktopServices::openUrl(QUrl(QDir::toNativeSeparators(link)));
    return;
  }
  if(link.startsWith(QLatin1String("www")))
    link.prepend(QStringLiteral("http://"));
  QDesktopServices::openUrl(QUrl(link));
}

void ChatMessageView::onRowsInserted(const QModelIndex& parent, int first, int last) {
  Q_UNUSED(parent);
  Q_UNUSED(first);
  Q_UNUSED(last);
  if(autoScroll)
    scrollToBottom();
  emit plainTextChanged();
}

void ChatMessageView::onPaletteChanged() {
  //  Live OS dark-theme switch without restart and without clearing the log.
  //  paletteChanged is gone under QT_DISABLE_DEPRECATED_BEFORE=0x060000 —
  //  catch QEvent::ApplicationPaletteChange instead of the signal.
  bool dark = qApp->palette().window().color().lightness() < 128;
  if(dark != systemDark) {
    systemDark = dark;
    bubbleDelegate->setMode(appearanceMode, systemDark);
    syncBackground();
    viewport()->update();
  }
}

QString ChatMessageView::appearanceModeString() const {
  switch(appearanceMode) {
  case ThemeV2::Mode::Light:
    return QStringLiteral("light");
  case ThemeV2::Mode::Dark:
    return QStringLiteral("dark");
  default:
    break;
  }
  return QStringLiteral("system");
}

bool ChatMessageView::event(QEvent* event) {
  if(event->type() == QEvent::ApplicationPaletteChange) {
    onPaletteChanged();
    return QListView::event(event);
  }
  if(event->type() == QEvent::KeyPress) {
    auto* ke = static_cast<QKeyEvent*>(event);
    if(ke->key() == Qt::Key_Tab && !(ke->modifiers() & Qt::ShiftModifier))
      return moveAnchorFocus(1);
    if(ke->key() == Qt::Key_Backtab ||
       (ke->key() == Qt::Key_Tab && (ke->modifiers() & Qt::ShiftModifier)))
      return moveAnchorFocus(-1);
    if((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) &&
       focusRow >= 0 && !focusHref.isEmpty()) {
      activateHref(focusHref);
      return true;
    }
    if(ke->matches(QKeySequence::Copy)) {
      copyAction_triggered();
      return true;
    }
  }
  return QListView::event(event);
}

QString ChatMessageView::themeFilePath() const {
  //  File-based theme for iteration without rebuild; fallback is the built-in qrc.
  QStringList candidates = {
    StdLocation::sysThemeDir() + QStringLiteral("/BubbleV2/theme.json"),
    StdLocation::userThemeDir() + QStringLiteral("/BubbleV2/theme.json"),
    QStringLiteral(":/themes/BubbleV2/theme.json")};
  for(const QString& path : candidates) {
    if(QFile::exists(path))
      return path;
  }
  return QStringLiteral(":/themes/BubbleV2/theme.json");
}

void ChatMessageView::syncBackground() {
  //  Empty viewport must match the per-row page fill in BubbleDelegate::paint:
  //  otherwise an empty chat is white (QPalette::Base) while rows sit on gray.
  bool darkMode = theme.isDark(appearanceMode, systemDark);
  QColor page = theme.resolveToken(QStringLiteral("page"), darkMode);
  if(!page.isValid())
    return;
  QPalette pal = palette();
  pal.setColor(QPalette::Base, page);
  pal.setColor(QPalette::Window, page);
  setPalette(pal);
  QPalette vp = viewport()->palette();
  vp.setColor(QPalette::Base, page);
  vp.setColor(QPalette::Window, page);
  viewport()->setPalette(vp);
  viewport()->setAutoFillBackground(true);
}
