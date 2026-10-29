// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "chatmessageview.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextDocument>
#include <QToolTip>
#include <QUrl>

#include "core/shared.h"
#include "core/stdlocation.h"
#include "core/lmccategories.h"
#include "gui/chat/chathelper.h"
#include "gui/chat/rowhtmlbuilder.h"
#include "gui/widgets/actionhelper.h"

namespace {

//  decodeMessage for file save: links detected, no trim, no UNC paths,
//  emoji-only spans (same as RowHtmlBuilder, single source for links/escape).
void decodeForSave(QString* text) {
  *text = RowHtmlBuilder::decodeBody(*text, false, true, false, QString());
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
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setUniformItemSizes(false);
  setSelectionMode(QAbstractItemView::NoSelection);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
  viewport()->setMouseTracking(true);
  setAcceptDrops(false);

  createContextMenu();
  connect(chatModel, &QAbstractItemModel::rowsInserted, this, &ChatMessageView::onRowsInserted);
  connect(chatModel, &QAbstractItemModel::rowsRemoved, this, &ChatMessageView::onRowsRemoved);
  connect(chatModel, &QAbstractItemModel::modelReset, this, &ChatMessageView::onModelReset);
  connect(chatModel, &QAbstractItemModel::modelReset, this,
          &ChatMessageView::plainTextChanged);
  connect(chatModel, &QAbstractItemModel::dataChanged, this,
          &ChatMessageView::plainTextChanged);
  connect(chatModel, &QAbstractItemModel::dataChanged, this, &ChatMessageView::onDataChanged);
  systemDark = qApp->palette().window().color().lightness() < 128;
}

void ChatMessageView::initChat(bool clearLog) {
  syncToLayers();
  if(clearLog)
    chatModel->clearLog();
  hasData = chatModel->hasData;
  reloadTheme();
}

void ChatMessageView::reloadTheme() {
  theme = Theme::loadFromFile(themeFilePath());
  if(!theme.isValid)
    theme = Theme::builtInDefault();
  bubbleDelegate->setTheme(theme);
  bubbleDelegate->setMode(appearanceMode, systemDark);
  setViewportMargins(theme.view.padding[3], theme.view.padding[0], theme.view.padding[1],
                     theme.view.padding[2]);
  syncBackground();
  requestRelayout();
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
  requestRelayout();
}

QString ChatMessageView::prepareMessageLogForSave(OutputFormat format) {
  const QList<SingleMessage> messages = chatModel->messages();
  if(format == HtmlFormat) {
    QString html =
      QStringLiteral("<html data-lmc-history=\"2\"><head><style type='text/css'>"
                     "*{font-size: 9pt;} body {-webkit-nbsp-mode: space; word-wrap: break-word;}"
                     "span.salutation {float:left; font-weight: bold;} span.time {float: right;}"
                     "span.message {clear: both; display: block;} p {border-bottom: 1px solid #CCC;}"
                     "</style></head><body>");
    for(const SingleMessage& msg : messages) {
      if(msg.type != MT_Message && msg.type != MT_GroupMessage)
        continue;
      XmlMessage xml = msg.message;
      // Numeric timestamp: no raw peer string reaches the saved attribute.
      const qint64 ts = xml.header(XN_TIME).toLongLong();
      QDateTime time;
      time.setMSecsSinceEpoch(ts);
      QString messageText = xml.data(XN_MESSAGE);
      if(messageText.isEmpty())
        messageText = xml.data(XN_GROUPMESSAGE);
      decodeForSave(&messageText);
      //  Per-message own/time/type for history bubbles (.html export ignores it).
      bool own = msg.userId == chatModel->localId;
      html += QStringLiteral("<p data-own=\"%1\" data-ts=\"%2\" data-type=\"%3\">"
                             "<span class='salutation'>%4:</span>"
                             "<span class='time'>%5</span>"
                             "<span class='message'>%6</span></p>")
                .arg(own ? QStringLiteral("1") : QStringLiteral("0"),
                     QString::number(ts), QString::number(int(msg.type)),
                     msg.userName.toHtmlEscaped(),
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

void ChatMessageView::showLegacyHtml(QString* userName, QDateTime* when, QString* html) {
  initChat();
  QTextDocument doc;
  doc.setHtml(html ? *html : QString());
  QString text = BubbleDelegate::normalizePlainText(doc.toPlainText()).trimmed();
  //  Broken offset or wiped historyFile: no phantom empty bubble.
  if(text.isEmpty())
    return;
  XmlMessage xmlMessage;
  if(when)
    xmlMessage.addHeader(XN_TIME, QString::number(when->toMSecsSinceEpoch()));
  xmlMessage.addData(XN_MESSAGE, text);
  //  Saved blobs carry no own-flag: history rows are always incoming
  //  (userId = userName never matches the empty history localId).
  QString userId = userName ? *userName : QString();
  appendMessageLog(MT_Message, &userId, userName, &xmlMessage);
}

namespace {

struct HistoryRecord {
  QString userName;
  bool own = false;
  qint64 ts = 0;
  MessageType type = MT_Message;
  QString body;
};

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

//  Envelope "2": one <p> per message; false when legacy or empty.
bool parseStructuredHistory(const QString& html, QList<HistoryRecord>* out) {
  if(!html.contains(QStringLiteral("data-lmc-history=\"2\"")))
    return false;
  static const QRegularExpression pRe(
    QStringLiteral("<p\\b([^>]*)>(.*?)</p>"),
    QRegularExpression::DotMatchesEverythingOption |
      QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatchIterator it = pRe.globalMatch(html);
  while(it.hasNext()) {
    QRegularExpressionMatch pm = it.next();
    HistoryRecord rec;
    rec.own = historyAttr(pm.captured(1), "data-own") == QLatin1String("1");
    rec.ts = historyAttr(pm.captured(1), "data-ts").toLongLong();
    rec.type = historyAttr(pm.captured(1), "data-type").toInt() == int(MT_GroupMessage)
                 ? MT_GroupMessage
                 : MT_Message;
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

} // namespace

bool ChatMessageView::isStructuredHistory(const QString* html) {
  return html && html->contains(QStringLiteral("data-lmc-history=\"2\""));
}

QString ChatMessageView::historyLocalId() {
  return QStringLiteral("lmc-history-local");
}

void ChatMessageView::showHistoryMessages(QString* userName, QDateTime* when, QString* html) {
  initChat();
  QList<HistoryRecord> records;
  if(html && parseStructuredHistory(*html, &records)) {
    QString local = historyLocalId();
    for(const HistoryRecord& rec : records) {
      XmlMessage xmlMessage;
      qint64 ts = rec.ts > 0 ? rec.ts : (when ? when->toMSecsSinceEpoch() : 0);
      xmlMessage.addHeader(XN_TIME, QString::number(ts));
      xmlMessage.addData(XN_MESSAGE, rec.body);
      //  Own rows key on the sentinel (right); others on sender name (left).
      QString userId = rec.own ? local : rec.userName;
      QString name = rec.userName;
      appendMessageLog(rec.type, &userId, &name, &xmlMessage);
    }
    return;
  }
  showLegacyHtml(userName, when, html);
}

bool ChatMessageView::isOwnRow(int row) const {
  if(row < 0 || row >= chatModel->rowCount())
    return false;
  return chatModel->data(chatModel->index(row, 0), ChatMessageModel::OwnRole).toBool();
}

void ChatMessageView::setAppearanceMode(const QString& newMode) {
  QString lower = newMode.toLower();
  if(lower == QLatin1String("light"))
    appearanceMode = Theme::Mode::Light;
  else if(lower == QLatin1String("dark"))
    appearanceMode = Theme::Mode::Dark;
  else
    appearanceMode = Theme::Mode::System;
  bubbleDelegate->setMode(appearanceMode, systemDark);
  syncBackground();
  requestRelayout();
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
  //  Stale row must not leak into later Ctrl+A (same slot, no menu open).
  connect(contextMenu, &QMenu::aboutToHide, this, [this] { contextRow = -1; });
  setContextMenuPolicy(Qt::CustomContextMenu);
}

void ChatMessageView::showContextMenu(const QPoint& pos) {
  QModelIndex menuRow = indexAt(pos);
  contextRow = menuRow.isValid() ? menuRow.row() : -1;
  if(!bubbleDelegate->hasTextSelection()) {
    if(menuRow.isValid())
      setCurrentIndex(menuRow);
  }
  copyAction->setEnabled(bubbleDelegate->hasTextSelection());
  selectAllAction->setEnabled(chatModel->rowCount() > 0);
  contextMenu->exec(mapToGlobal(pos));
}

void ChatMessageView::copyAction_triggered() {
  QString text = bubbleDelegate->selectedText();
  if(text.isEmpty())
    return;
  QApplication::clipboard()->setText(text);
}

void ChatMessageView::selectAllAction_triggered() {
  QModelIndex target;
  //  Open menu wins: Select All means "the whole bubble under the cursor",
  //  not a stale selection from another bubble.
  if(contextRow >= 0)
    target = chatModel->index(contextRow, 0);
  if(!target.isValid()) {
    if(bubbleDelegate->hasTextSelection())
      target = chatModel->index(bubbleDelegate->selectionRow(), 0);
    else
      target = currentIndex().isValid() ? currentIndex()
                                        : chatModel->index(chatModel->rowCount() - 1, 0);
  }
  if(!target.isValid())
    return;
  setCurrentIndex(target);
  bubbleDelegate->selectAllText(target, indexOption(target));
  viewport()->update();
}

QStyleOptionViewItem ChatMessageView::indexOption(const QModelIndex& idx) const {
  QStyleOptionViewItem opt;
  opt.initFrom(this);
  opt.rect = visualRect(idx);
  opt.widget = viewport();
  return opt;
}

QModelIndex ChatMessageView::textIndexAt(const QPoint& pos, int* charPos) const {
  QModelIndex idx = indexAt(pos);
  if(!idx.isValid())
    return QModelIndex();
  QStyleOptionViewItem opt = indexOption(idx);
  int hit = bubbleDelegate->charPosAt(pos, opt, idx);
  if(hit < 0)
    return QModelIndex();
  if(charPos)
    *charPos = hit;
  return idx;
}

void ChatMessageView::clearTextSelection() {
  const bool had = bubbleDelegate->hasTextSelection() || textSelecting;
  bubbleDelegate->clearTextSelection();
  textSelecting = false;
  if(had)
    viewport()->update();
}
QModelIndex ChatMessageView::anchorRowAt(const QPoint& pos, QString* href) {
  QModelIndex idx = indexAt(pos);
  if(!idx.isValid())
    return QModelIndex();
  QStyleOptionViewItem opt;
  opt.initFrom(this);
  opt.rect = visualRect(idx);
  opt.widget = viewport();
  QString found = bubbleDelegate->anchorAt(pos, opt, idx);
  if(found.isEmpty())
    return QModelIndex();
  if(href)
    *href = found;
  return idx;
}

void ChatMessageView::mousePressEvent(QMouseEvent* event) {
  if(event->button() == Qt::RightButton) {
    QModelIndex menuRow = indexAt(event->pos());
    if(menuRow.isValid()) {
      //  Foreign selection does not survive a right-click on another
      //  bubble (QTextEdit-like); same-row selection stays for Copy.
      if(bubbleDelegate->hasTextSelection() &&
         menuRow.row() != bubbleDelegate->selectionRow())
        clearTextSelection();
      setCurrentIndex(menuRow);
    }
    QListView::mousePressEvent(event);
    return;
  }
  if(event->button() != Qt::LeftButton) {
    QListView::mousePressEvent(event);
    return;
  }
  QString href;
  QModelIndex anchorIdx = anchorRowAt(event->pos(), &href);
  if(anchorIdx.isValid() && !href.isEmpty()) {
    clearTextSelection();
    setCurrentIndex(anchorIdx);
    activateHref(href);
    return;
  }
  int charPos = -1;
  QModelIndex textIdx = textIndexAt(event->pos(), &charPos);
  bubbleDelegate->clearFocusAnchor();
  focusRow = -1;
  focusHref.clear();
  if(textIdx.isValid()) {
    setCurrentIndex(textIdx);
    bubbleDelegate->setTextSelection(textIdx.row(), charPos, charPos);
    textSelecting = true;
    viewport()->update();
    return;
  }
  clearTextSelection();
  QListView::mousePressEvent(event);
}

void ChatMessageView::mouseMoveEvent(QMouseEvent* event) {
  if(textSelecting && (event->buttons() & Qt::LeftButton)) {
    int row = bubbleDelegate->selectionRow();
    QModelIndex idx = chatModel->index(row, 0);
    if(idx.isValid()) {
      int clamped = bubbleDelegate->charPosClamped(event->pos(), indexOption(idx), idx);
      if(clamped >= 0) {
        bubbleDelegate->extendSelection(clamped);
        viewport()->update();
      }
    }
    viewport()->setCursor(Qt::IBeamCursor);
    return;
  }
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
    QToolTip::showText(viewport()->mapToGlobal(event->pos()), tip, this);
  } else if(textIndexAt(event->pos()).isValid()) {
    viewport()->setCursor(Qt::IBeamCursor);
    QToolTip::hideText();
  } else {
    viewport()->unsetCursor();
    QToolTip::hideText();
  }
  QListView::mouseMoveEvent(event);
}

void ChatMessageView::mouseReleaseEvent(QMouseEvent* event) {
  if(textSelecting && event->button() == Qt::LeftButton) {
    textSelecting = false;
    if(!bubbleDelegate->hasTextSelection())
      bubbleDelegate->clearTextSelection();
    viewport()->unsetCursor();
    viewport()->update();
    return;
  }
  QListView::mouseReleaseEvent(event);
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
  QStyleOptionViewItem opt;
  opt.initFrom(this);
  opt.widget = viewport();
  opt.rect = QRect(0, 0, viewport()->width(), 0);
  if(focusRow < 0) {
    int start = (delta > 0) ? 0 : rows - 1;
    int step = (delta > 0) ? 1 : -1;
    for(int r = start; r >= 0 && r < rows; r += step) {
      QStringList names = bubbleDelegate->anchors(chatModel->index(r, 0), opt);
      if(!names.isEmpty()) {
        focusRow = r;
        focusHref = (delta > 0) ? names.first() : names.last();
        break;
      }
    }
  } else {
    QStringList current = bubbleDelegate->anchors(chatModel->index(focusRow, 0), opt);
    int pos = current.indexOf(focusHref);
    if(pos + delta >= 0 && pos + delta < current.size()) {
      focusHref = current.at(pos + delta);
    } else {
      //  Linear scan to the list edge, no wrap-around: past the last anchor
      //  Tab must leave the view for the input/Send button (a11y, no trap).
      int r = focusRow + delta;
      bool found = false;
      while(r >= 0 && r < rows) {
        QStringList names = bubbleDelegate->anchors(chatModel->index(r, 0), opt);
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
  if(focusRow >= 0) {
    clearTextSelection();
    scrollTo(chatModel->index(focusRow, 0));
  }
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
  Q_UNUSED(last);
  //  Deliberately broad: an insert at/above the selection shifts content
  //  down, so the old index would mistarget another bubble; without a
  //  follow-shift API, dropping the highlight is the safe choice.
  if(bubbleDelegate->hasTextSelection() && bubbleDelegate->selectionRow() >= first)
    clearTextSelection();
  if(autoScroll)
    scrollToBottom();
  emit plainTextChanged();
}

void ChatMessageView::onRowsRemoved(const QModelIndex& parent, int first, int last) {
  Q_UNUSED(parent);
  //  Only a hit selection dies; removals above would shift rows, but no
  //  such producer exists (the typing row is always trailing, the rest
  //  goes through modelReset).
  if(!bubbleDelegate->hasTextSelection())
    return;
  const int row = bubbleDelegate->selectionRow();
  if(row >= first && row <= last)
    clearTextSelection();
}

void ChatMessageView::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                    const QList<int>& roles) {
  if(!bubbleDelegate->hasTextSelection())
    return;
  const int row = bubbleDelegate->selectionRow();
  if(row < topLeft.row() || row > bottomRight.row())
    return;
  //  Avatar-only updates keep the highlight; text changes drop it.
  if(!roles.isEmpty() && !roles.contains(Qt::DisplayRole) &&
     !roles.contains(ChatMessageModel::TextRole) &&
     !roles.contains(ChatMessageModel::FileOpRole))
    return;
  clearTextSelection();
}

void ChatMessageView::onModelReset() {
  bubbleDelegate->invalidateCache();
  bubbleDelegate->clearTextSelection();
  textSelecting = false;
  requestRelayout();
}

void ChatMessageView::resizeEvent(QResizeEvent* event) {
  QListView::resizeEvent(event);
  bubbleDelegate->noteViewportWidth(viewport()->width());
  requestRelayout();
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
    requestRelayout();
  }
}

void ChatMessageView::setThemeName(const QString& name) {
  if(name.isEmpty() || name == themeName)
    return;
  if(Theme::themeFileFor(name).isEmpty()) {
    //  Stale INI (e.g. removed theme): fall back to the default
    //  instead of keeping an unresolvable name.
    qCWarning(lmcUi) << "ChatMessageView: unknown theme" << name;
    themeName = QStringLiteral("ClassicSlate");
    reloadTheme();
    return;
  }
  themeName = name;
  reloadTheme();
}

QString ChatMessageView::themeNameString() const {
  return themeName;
}

QString ChatMessageView::appearanceModeString() const {
  switch(appearanceMode) {
  case Theme::Mode::Light:
    return QStringLiteral("light");
  case Theme::Mode::Dark:
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
    if(ke->matches(QKeySequence::SelectAll)) {
      selectAllAction_triggered();
      return true;
    }
  }
  return QListView::event(event);
}

QString ChatMessageView::themeFilePath() const {
  //  Name-resolved (user dir wins over built-in qrc); file-based so theme
  //  authors can iterate without rebuild. Fallback is the built-in qrc.
  QString file = Theme::themeFileFor(themeName);
  if(!file.isEmpty())
    return file;
  return QStringLiteral(":/themes/ClassicSlate.json");
}

void ChatMessageView::syncBackground() {
  //  Empty viewport must match the per-row page fill in BubbleDelegate::paint:
  //  otherwise an empty chat is white (QPalette::Base) while rows sit on gray.
  //  The viewPadding strip from setViewportMargins belongs to the view itself
  //  (outside viewport()), so the view must autofill too — otherwise the
  //  parent shows through and the padding reads as a foreign-color margin.
  bool darkMode = Theme::isDark(appearanceMode, systemDark);
  QColor page = Theme::pick(theme.page, darkMode);
  if(!page.isValid())
    return;
  QPalette pal = palette();
  pal.setColor(QPalette::Base, page);
  pal.setColor(QPalette::Window, page);
  setPalette(pal);
  setAutoFillBackground(true);
  QPalette vp = viewport()->palette();
  vp.setColor(QPalette::Base, page);
  vp.setColor(QPalette::Window, page);
  viewport()->setPalette(vp);
  viewport()->setAutoFillBackground(true);
}

void ChatMessageView::requestRelayout() {
  scheduleDelayedItemsLayout();
  viewport()->update();
}
