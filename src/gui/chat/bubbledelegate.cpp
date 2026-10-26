// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "bubbledelegate.h"

#include <QAbstractTextDocumentLayout>
#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QTextCursor>
#include <QUrl>
#include <QtMath>

#include "core/shared.h"
#include "core/uidefinitions.h"
#include "gui/chat/anchornavigator.h"
#include "gui/chat/chatmessagemodel.h"
#include "gui/chat/rowhtmlbuilder.h"

namespace {

const int kMaxCacheEntries = 150;

//  Per-corner rounded rect: c is [TL, TR, BR, BL] in px, clamped to the rect.
QPainterPath roundedCorners(const QRect& r, const int c[4]) {
  int lim = qMin(r.width(), r.height()) / 2;
  int tl = qBound(0, c[0], lim);
  int tr = qBound(0, c[1], lim);
  int br = qBound(0, c[2], lim);
  int bl = qBound(0, c[3], lim);
  QPainterPath path;
  path.moveTo(r.left() + tl, r.top());
  path.lineTo(r.right() - tr, r.top());
  if(tr > 0)
    path.arcTo(r.right() - 2 * tr, r.top(), 2 * tr, 2 * tr, 90, -90);
  path.lineTo(r.right(), r.bottom() - br);
  if(br > 0)
    path.arcTo(r.right() - 2 * br, r.bottom() - 2 * br, 2 * br, 2 * br, 0, -90);
  path.lineTo(r.left() + bl, r.bottom());
  if(bl > 0)
    path.arcTo(r.left(), r.bottom() - 2 * bl, 2 * bl, 2 * bl, 270, -90);
  path.lineTo(r.left(), r.top() + tl);
  if(tl > 0)
    path.arcTo(r.left(), r.top(), 2 * tl, 2 * tl, 180, -90);
  path.closeSubpath();
  return path;
}

} // namespace

BubbleDelegate::BubbleDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

void BubbleDelegate::setTheme(const Theme& newTheme) {
  theme = newTheme;
  invalidateCache();
}

void BubbleDelegate::setMode(Theme::Mode newMode, bool newSystemDark) {
  mode = newMode;
  systemDark = newSystemDark;
  invalidateCache();
}

void BubbleDelegate::setDisplaySettings(const DisplaySettings& settings) {
  if(display == settings)
    return;
  display = settings;
  invalidateCache();
}

void BubbleDelegate::invalidateCache() {
  for(auto it = cache.begin(); it != cache.end(); ++it)
    delete it.value().doc;
  cache.clear();
  avatars.clear();
}

void BubbleDelegate::noteViewportWidth(int viewportWidth) {
  for(auto it = cache.begin(); it != cache.end();) {
    if(it.value().width != viewportWidth) {
      delete it.value().doc;
      it = cache.erase(it);
    } else {
      ++it;
    }
  }
}

double BubbleDelegate::localPointSize() const {
  static const double sizes[] = {8.25, 9.0, 10.5};
  return sizes[qBound(0, display.fontSizeVal, FS_COUNT - 1)];
}

QFont BubbleDelegate::headerNameFont() const {
  //  No index: the message font cannot leak into the header.
  QFont f;
  f.setPointSizeF(localPointSize() * theme.header.senderSize);
  f.setWeight(static_cast<QFont::Weight>(theme.header.senderWeight));
  return f;
}

QFont BubbleDelegate::headerTimeFont() const {
  QFont f;
  f.setPointSizeF(localPointSize() * theme.header.timeSize);
  return f;
}

QString BubbleDelegate::rowTime(const QModelIndex& index) const {
  if(!display.messageTime)
    return QString();
  QDateTime t = index.data(ChatMessageModel::TimeRole).toDateTime();
  if(!t.isValid())
    return QString();
  //  Fixed breaking change: HH:mm from the theme, no longer QLocale::ShortFormat.
  QString time = t.toString(theme.header.timeFormat.isEmpty() ? QStringLiteral("HH:mm")
                                                              : theme.header.timeFormat);
  if(display.messageDate)
    time = t.toString(QStringLiteral("dd.MM.yyyy ")) + time;
  return time;
}

QString BubbleDelegate::fileCardHtml(const QModelIndex& index) const {
  QString userName = index.data(ChatMessageModel::UserNameRole).toString();
  QString fileName = index.data(ChatMessageModel::FileNameRole).toString();
  qint64 fileSize = index.data(ChatMessageModel::FileSizeRole).toLongLong();
  QString fileOp = index.data(ChatMessageModel::FileOpRole).toString();
  QString fileMode = index.data(ChatMessageModel::FileModeRole).toString();
  QString fileId;
  {
    //  Model id is a tempId (send/receive+fileId); anchors need the bare fileId.
    QString tempId = index.data(ChatMessageModel::IdRole).toString();
    fileId = tempId;
    if(tempId.startsWith(QStringLiteral("send")))
      fileId = tempId.mid(4);
    else if(tempId.startsWith(QStringLiteral("receive")))
      fileId = tempId.mid(7);
  }
  int type = index.data(ChatMessageModel::TypeRole).toInt();
  //  URL token, never translated (compared against literals in activateHref).
  QString kind = (type == MT_Folder) ? QStringLiteral("folder") : QStringLiteral("file");
  QString caption;
  if(fileMode == QLatin1String("receive"))
    caption = (type == MT_Folder) ? tr("%1 sends you a folder:").arg(userName)
                                  : tr("%1 sends you a file:").arg(userName);
  else
    caption = tr("Sending '%1' to %2.").arg(fileName, userName);
  QString html = QStringLiteral("<b>") + caption.toHtmlEscaped() + QStringLiteral("</b><br/>") +
                 fileName.toHtmlEscaped() + QStringLiteral(" (") +
                 Helper::formatSize(fileSize) + QStringLiteral(")<br/>");
  FileOp op = (FileOp)Helper::indexOf(FileOpNames, FO_Max, fileOp);
  if(op == FO_Request) {
    if(fileMode == QLatin1String("receive")) {
      if(display.autoFile) {
        html += tr("Accepted");
      } else {
        html += QStringLiteral("<a href='lmc://%1/accept/%2'>%3</a>&nbsp;&nbsp;"
                               "<a href='lmc://%1/decline/%2'>%4</a>")
                  .arg(kind, fileId, tr("Accept"), tr("Decline"));
      }
    } else {
      html += QStringLiteral("<a href='lmc://%1/cancel/%2'>%3</a>")
                .arg(kind, fileId, tr("Cancel"));
    }
  } else {
    QString status;
    switch(op) {
    case FO_Accept:
      status = tr("Accepted");
      break;
    case FO_Decline:
      status = tr("Declined");
      break;
    case FO_Cancel:
      status = tr("Canceled");
      break;
    case FO_Error:
    case FO_Abort:
      status = tr("Interrupted");
      break;
    case FO_Complete:
      status = tr("Completed");
      break;
    default:
      break;
    }
    html += status.toHtmlEscaped();
  }
  return html;
}

BubbleDelegate::ResolvedRow BubbleDelegate::resolveRow(const QModelIndex& index) const {
  const int type = index.data(ChatMessageModel::TypeRole).toInt();
  const bool own = index.data(ChatMessageModel::OwnRole).toBool();
  const bool grouped = index.data(ChatMessageModel::GroupedRole).toBool();
  return RowResolver::resolve(theme, RowResolver::roleForCard(type, own), grouped);
}

QString BubbleDelegate::rowHtml(const QModelIndex& index, const ResolvedRow& row) const {
  int type = index.data(ChatMessageModel::TypeRole).toInt();
  bool isFile = (type == MT_File || type == MT_Folder);
  const bool darkMode = Theme::isDark(mode, systemDark);
  QFont font = RowHtmlBuilder::messageFont(index.data(ChatMessageModel::FontRole),
                                           localPointSize());
  QColor color =
    RowHtmlBuilder::guardedColor(index.data(ChatMessageModel::ColorRole).value<QColor>(),
                                 row.bg, row.text, theme.page, darkMode);
  bool own = index.data(ChatMessageModel::OwnRole).toBool();
  bool fontExplicit = index.data(ChatMessageModel::FontRole).isValid();
  QString style = RowHtmlBuilder::fontStyleFor(font, color, display.fontSizeVal, own,
                                               fontExplicit);
  QString linkColorName = Theme::pick(theme.link, darkMode).name();
  QString body;
  if(isFile) {
    body = fileCardHtml(index);
  } else {
    body = RowHtmlBuilder::decodeBody(index.data(ChatMessageModel::TextRole).toString(),
                                      display.trimMessage, display.allowLinks,
                                      display.pathToLink, display.showSmiley);
  }
  return QStringLiteral("<div style='%1'>%2</div>").arg(style, body) +
         QStringLiteral("<style>a { color: %1; }</style>").arg(linkColorName);
}

int BubbleDelegate::viewWidth(const QStyleOptionViewItem& option) const {
  if(const QListView* view = qobject_cast<const QListView*>(parent()))
    return view->viewport()->width();
  return option.rect.width();
}

QRect BubbleDelegate::rowBubbleRect(int viewportWidth, int contentH,
                                    const ResolvedRow& row) const {
  const int contentW =
    BubbleGeometry::contentWidth(row, viewportWidth, theme.avatar.size);
  RowGeometry geo = BubbleGeometry::buildLayout(row, viewportWidth, contentW, contentH,
                                                headerNameFont(), headerTimeFont(),
                                                theme.avatar.size);
  return geo.bubbleRect;
}

QString BubbleDelegate::contentRev(const QModelIndex& index) const {
  //  Text part is a lazy-once model signature; the rest is small fixed-size state.
  int textLen = 0;
  size_t textHash = 0;
  if(const ChatMessageModel* model = qobject_cast<const ChatMessageModel*>(index.model())) {
    textHash = model->rowTextHash(index.row(), &textLen);
  } else {
    //  Non-model/proxy path: full TextRole hash on purpose, may differ from body-only rev.
    QString text = index.data(ChatMessageModel::TextRole).toString();
    textLen = text.length();
    textHash = qHash(text);
  }
  QString avatar = index.data(ChatMessageModel::AvatarRole).toString();
  QString fileOp = index.data(ChatMessageModel::FileOpRole).toString();
  QString user = index.data(ChatMessageModel::UserNameRole).toString();
  QString time = index.data(ChatMessageModel::TimeRole).toDateTime().toString(Qt::ISODate);
  return QStringLiteral("%1|%2|%3|%4|%5|%6")
    .arg(textLen)
    .arg(static_cast<quint64>(textHash))
    .arg(static_cast<quint64>(qHash(avatar + fileOp + user + time)))
    .arg(index.data(ChatMessageModel::GroupedRole).toBool())
    .arg(index.data(ChatMessageModel::OwnRole).toBool())
    .arg(index.data(ChatMessageModel::TypeRole).toInt());
}

QString BubbleDelegate::cacheKey(const QModelIndex& index, int viewportWidth) const {
  qreal dpr = 1.0;
  if(QWidget* w = qobject_cast<QWidget*>(parent()))
    dpr = w->devicePixelRatioF();
  return QStringLiteral("%1|%2|%3|%4")
    .arg(index.row())
    .arg(viewportWidth)
    .arg(int(dpr * 100))
    .arg(contentRev(index));
}

const BubbleDelegate::CacheEntry& BubbleDelegate::entryFor(const QModelIndex& index,
                                                           int viewportWidth) const {
  QString key = cacheKey(index, viewportWidth);
  auto it = cache.constFind(key);
  if(it != cache.constEnd())
    return it.value();
  if(cache.size() >= kMaxCacheEntries) {
    for(auto it = cache.begin(); it != cache.end(); ++it)
      delete it.value().doc;
    cache.clear();
  }
  CacheEntry e;
  e.width = viewportWidth;
  e.doc = new QTextDocument(const_cast<BubbleDelegate*>(this));
  //  Bare QTextDocument has no searchPaths (unlike QTextBrowser),
  //  so qrc smileys are registered explicitly: addResource is the only path.
  for(int i = 0; i < SM_MAPCOUNT; i++) {
    QString key = QStringLiteral("qrc") + smileyPic[i];
    if(!avatars.contains(key)) {
      QPixmap pm(smileyPic[i]);
      if(!pm.isNull())
        avatars.insert(key, pm);
    }
    if(avatars.contains(key))
      e.doc->addResource(QTextDocument::ImageResource, QUrl(key), avatars.value(key));
  }
  ResolvedRow row = resolveRow(index);
  e.doc->setHtml(rowHtml(index, row));
  const int contentW =
    BubbleGeometry::contentWidth(row, viewportWidth, theme.avatar.size);
  e.doc->setTextWidth(contentW);
  QSizeF docSize = e.doc->size();
  e.row = row;
  e.geo = BubbleGeometry::buildLayout(row, viewportWidth, contentW, qCeil(docSize.height()),
                                      headerNameFont(), headerTimeFont(), theme.avatar.size);
  cache.insert(key, e);
  return cache.constFind(key).value();
}

QPixmap BubbleDelegate::avatarPixmap(const QString& path, int size) const {
  QString key = QStringLiteral("%1|%2").arg(path).arg(size);
  auto it = avatars.constFind(key);
  if(it != avatars.constEnd())
    return it.value();
  QPixmap pm;
  if(!path.isEmpty() && QFile::exists(path))
    pm.load(path);
  if(pm.isNull())
    pm = QPixmap(QLatin1String(":/avatars/default"));
  QPixmap scaled =
    pm.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
  QPixmap circle(size, size);
  circle.fill(Qt::transparent);
  QPainter p(&circle);
  p.setRenderHint(QPainter::Antialiasing);
  QPainterPath clip;
  if(theme.avatar.shape == QLatin1String("square")) {
    clip.addRect(0, 0, size, size);
  } else if(theme.avatar.shape == QLatin1String("rounded")) {
    clip.addRoundedRect(0, 0, size, size, size / 4.0, size / 4.0);
  } else {
    clip.addEllipse(0, 0, size, size);
  }
  p.setClipPath(clip);
  p.drawPixmap(0, 0, scaled);
  p.end();
  avatars.insert(key, circle);
  return circle;
}

void BubbleDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                           const QModelIndex& index) const {
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);
  bool darkMode = Theme::isDark(mode, systemDark);
  const int width = viewWidth(option);
  const CacheEntry& e = entryFor(index, width);
  const ResolvedRow& row = e.row;
  const QString& card = row.card;
  int top = option.rect.top();

  painter->fillRect(option.rect, Theme::pick(theme.page, darkMode));

  //  Header runs come from the layout header node (items + color
  //  overrides); series-start trees carry it, _next trees omit it.
  //  The theme header holds defaults. Name run is senderColor,
  //  time run is timeColor and always regular.
  if(row.header) {
    QFont nf = headerNameFont();
    QFont tf = headerTimeFont();
    QString time = rowTime(index);
    bool wantSender = row.headerItems.contains(QStringLiteral("sender"));
    bool wantTime = row.headerItems.contains(QStringLiteral("time"));
    QString namePart;
    QString timePart = wantTime ? time : QString();
    if(card == QLatin1String("incoming")) {
      if(wantSender)
        namePart = index.data(ChatMessageModel::UserNameRole).toString();
      if(!namePart.isEmpty() && !timePart.isEmpty())
        timePart = QStringLiteral(" ") + timePart;
    } else if(card == QLatin1String("outgoing")) {
      if(theme.header.outgoingYouLabel) {
        namePart = tr("You");
        if(!timePart.isEmpty())
          timePart = QStringLiteral(" ") + timePart;
      }
      //  The "You" prefix stays regular even when the incoming name is semibold.
      nf.setWeight(QFont::Normal);
    }
    QColor nameColor = Theme::pick(row.senderColor, darkMode);
    QColor timeColor = Theme::pick(row.timeColor, darkMode);
    QFontMetrics nfm(nf);
    QFontMetrics tfm(tf);
    int hx = e.geo.bubbleRect.left();
    if(card == QLatin1String("outgoing"))
      hx = e.geo.bubbleRect.right() - nfm.horizontalAdvance(namePart) -
           tfm.horizontalAdvance(timePart);
    int baseline = top + row.marginTop + e.geo.headerAscent;
    if(!namePart.isEmpty()) {
      painter->setFont(nf);
      painter->setPen(nameColor);
      painter->drawText(hx, baseline, namePart);
      hx += nfm.horizontalAdvance(namePart);
    }
    if(!timePart.isEmpty()) {
      painter->setFont(tf);
      painter->setPen(timeColor);
      painter->drawText(hx, baseline, timePart);
    }
  }

  QRect bubble = e.geo.bubbleRect.translated(0, top);
  QRect docRect = e.geo.docRect.translated(0, top);

  if(row.avatar) {
    QString avatarPath = index.data(ChatMessageModel::AvatarRole).toString();
    QPixmap pm = avatarPixmap(avatarPath, theme.avatar.size);
    int ay = bubble.top() + qMax(0, (bubble.height() - theme.avatar.size) / 2);
    painter->drawPixmap(0, ay, pm);
  }

  if(row.shadowOpacity > 0.0) {
    QColor shadow(0, 0, 0);
    shadow.setAlphaF(row.shadowOpacity);
    painter->setPen(Qt::NoPen);
    painter->setBrush(shadow);
    QRect shadowRect = bubble.translated(int(row.shadowDx), int(row.shadowDy));
    painter->drawPath(roundedCorners(shadowRect, row.corners));
  }

  QColor bg, bg2;
  Theme::pickBg(row.bg, darkMode, bg, bg2);
  QLinearGradient grad(bubble.topLeft(), bubble.bottomLeft());
  grad.setColorAt(0, bg);
  grad.setColorAt(1, bg2);
  painter->setBrush(grad);
  if(row.hasBorder) {
    painter->setPen(Theme::pick(row.border, darkMode));
  } else {
    painter->setPen(Qt::NoPen);
  }
  painter->drawPath(roundedCorners(bubble, row.corners));

  //  Body: a single QTextDocument chunk in document coordinates.
  //  With a fragment selection the layout draws with a Highlight selection
  //  instead of the plain drawContents path.
  painter->save();
  painter->setClipRect(bubble);
  painter->translate(docRect.topLeft());
  int selLo = qMin(selStart, selEnd);
  int selHi = qMax(selStart, selEnd);
  bool hasSel = (index.row() == selRow && selLo != selHi && e.doc != nullptr);
  if(!hasSel) {
    e.doc->drawContents(painter, QRectF(0, 0, docRect.width(), docRect.height()));
  } else {
    int len = e.doc->characterCount();
    selLo = qBound(0, selLo, qMax(0, len - 1));
    selHi = qBound(0, selHi, qMax(0, len - 1));
    QTextCursor cursor(e.doc);
    cursor.setPosition(selLo);
    cursor.setPosition(selHi, QTextCursor::KeepAnchor);
    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette = option.palette;
    ctx.clip = QRectF(0, 0, docRect.width(), docRect.height());
    QAbstractTextDocumentLayout::Selection sel;
    sel.cursor = cursor;
    sel.format.setBackground(option.palette.color(QPalette::Highlight));
    sel.format.setForeground(option.palette.color(QPalette::HighlightedText));
    ctx.selections.append(sel);
    e.doc->documentLayout()->draw(painter, ctx);
  }
  painter->restore();

  if(index.row() == focusRow && !focusHref.isEmpty()) {
    QRect focusRect = anchorRect(focusHref, option, index);
    if(!focusRect.isEmpty()) {
      painter->setPen(QPen(Theme::pick(theme.link, darkMode), 2));
      painter->setBrush(Qt::NoBrush);
      painter->drawRoundedRect(focusRect.adjusted(-2, -1, 2, 1), 3, 3);
    }
  }
  painter->restore();
}

QSize BubbleDelegate::sizeHint(const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
  const CacheEntry& e = entryFor(index, viewWidth(option));
  return e.geo.size;
}

QString BubbleDelegate::anchorAt(const QPoint& pos, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
  const CacheEntry& e = entryFor(index, viewWidth(option));
  return AnchorNavigator::anchorAt(e.doc, e.geo.docRect.translated(0, option.rect.top()),
                                   pos);
}

QRect BubbleDelegate::anchorRect(const QString& href, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
  const CacheEntry& e = entryFor(index, viewWidth(option));
  return AnchorNavigator::anchorRect(e.doc, e.geo.docRect.translated(0, option.rect.top()),
                                     href);
}

void BubbleDelegate::setFocusAnchor(int row, const QString& href) {
  focusRow = row;
  focusHref = href;
}

void BubbleDelegate::clearFocusAnchor() {
  focusRow = -1;
  focusHref.clear();
}

QStringList BubbleDelegate::anchors(const QModelIndex& index,
                                    const QStyleOptionViewItem& option) const {
  const CacheEntry& e = entryFor(index, viewWidth(option));
  return AnchorNavigator::anchors(e.doc);
}

int BubbleDelegate::charPosAt(const QPoint& pos, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
  const CacheEntry& e = entryFor(index, viewWidth(option));
  QRect docRect = e.geo.docRect.translated(0, option.rect.top());
  if(!docRect.contains(pos) || e.doc == nullptr)
    return -1;
  QPointF docPos = QPointF(pos - docRect.topLeft());
  int hit = e.doc->documentLayout()->hitTest(docPos, Qt::FuzzyHit);
  if(hit < 0)
    return -1;
  return qBound(0, hit, qMax(0, e.doc->characterCount() - 1));
}

int BubbleDelegate::charPosClamped(const QPoint& pos, const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
  const CacheEntry& e = entryFor(index, viewWidth(option));
  if(e.doc == nullptr)
    return -1;
  int len = qMax(0, e.doc->characterCount() - 1);
  QRect docRect = e.geo.docRect.translated(0, option.rect.top());
  if(pos.y() < docRect.top())
    return 0;
  if(pos.y() >= docRect.bottom())
    return len;
  int hit = e.doc->documentLayout()->hitTest(QPointF(pos - docRect.topLeft()), Qt::FuzzyHit);
  if(hit < 0)
    return len;
  return qBound(0, hit, len);
}

void BubbleDelegate::setTextSelection(int row, int start, int end) {
  selRow = row;
  selStart = start;
  selEnd = end;
}

void BubbleDelegate::extendSelection(int pos) {
  if(selRow < 0)
    return;
  selEnd = pos;
}

void BubbleDelegate::clearTextSelection() {
  selRow = -1;
  selStart = 0;
  selEnd = 0;
}

bool BubbleDelegate::hasTextSelection() const {
  return selRow >= 0 && selStart != selEnd;
}

int BubbleDelegate::selectionRow() const {
  return selRow;
}

QString BubbleDelegate::selectedText() const {
  if(selRow < 0 || selStart == selEnd)
    return QString();
  const QListView* view = qobject_cast<const QListView*>(parent());
  if(view == nullptr || view->model() == nullptr || view->viewport() == nullptr)
    return QString();
  QModelIndex idx = view->model()->index(selRow, 0);
  if(!idx.isValid())
    return QString();
  const CacheEntry& e = entryFor(idx, view->viewport()->width());
  int len = qMax(0, e.doc->characterCount() - 1);
  int lo = qBound(0, qMin(selStart, selEnd), len);
  int hi = qBound(0, qMax(selStart, selEnd), len);
  if(lo == hi)
    return QString();
  QTextCursor cursor(e.doc);
  cursor.setPosition(lo);
  cursor.setPosition(hi, QTextCursor::KeepAnchor);
  QString text = cursor.selectedText();
  //  QTextDocument keeps HTML spaces as U+00A0 and blocks as U+2029;
  //  plain-text copy wants regular spaces and newlines.
  text.replace(QChar(0x00A0), QLatin1Char(' '));
  text.replace(QChar(0x2029), QLatin1Char('\n'));
  return text;
}

void BubbleDelegate::selectAllText(const QModelIndex& index,
                                   const QStyleOptionViewItem& option) {
  if(!index.isValid())
    return;
  const CacheEntry& e = entryFor(index, viewWidth(option));
  int len = qMax(0, e.doc->characterCount() - 1);
  selRow = index.row();
  selStart = 0;
  selEnd = len;
}
