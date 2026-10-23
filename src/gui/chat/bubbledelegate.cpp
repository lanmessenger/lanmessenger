// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "bubbledelegate.h"

#include <QPainter>
#include <QPainterPath>
#include <QAbstractTextDocumentLayout>
#include <QFile>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextLayout>
#include <QUrl>
#include <QtMath>

#include "core/shared.h"
#include "core/uidefinitions.h"
#include "gui/chat/chatmessagemodel.h"

namespace {

const int kAvatarGap = 8;
const int kRowMargin = 8;
const int kHeaderGap = 2;
const int kMaxCacheEntries = 150;

QString fontStyleFor(const QFont& font, const QColor& color, int fontSizeVal, bool own,
                     bool fontExplicit) {
  QString style = QStringLiteral("font-family:\"%1\"; ").arg(font.family());
  if(font.italic())
    style += QStringLiteral("font-style:italic; ");
  if(font.bold())
    style += QStringLiteral("font-weight:bold; ");
  //  An explicitly chosen sender font is honored with its own size (like color);
  //  otherwise the size comes from the local theme setting.
  if(own || fontExplicit) {
    style += QStringLiteral("font-size:%1pt; ").arg(font.pointSizeF());
    if(color.isValid())
      style += QStringLiteral("color:%1; ").arg(color.name());
  } else {
    style += fontStyle[qBound(0, fontSizeVal, FS_COUNT - 1)] + QStringLiteral(" ");
    if(color.isValid())
      style += QStringLiteral("color:%1; ").arg(color.name());
  }
  if(font.strikeOut())
    style += QStringLiteral("text-decoration:line-through; ");
  if(font.underline())
    style += QStringLiteral("text-decoration:underline; ");
  return style;
}

} // namespace

BubbleDelegate::BubbleDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

void BubbleDelegate::setTheme(const Theme& newTheme) {
  theme = newTheme;
  themeRev++;
  invalidateCache();
}

void BubbleDelegate::setMode(Theme::Mode newMode, bool newSystemDark) {
  mode = newMode;
  systemDark = newSystemDark;
  themeRev++;
  invalidateCache();
}

void BubbleDelegate::setDisplaySettings(const DisplaySettings& settings) {
  if(display == settings)
    return;
  display = settings;
  themeRev++;
  invalidateCache();
}

void BubbleDelegate::invalidateCache() {
  for(auto it = cache.begin(); it != cache.end(); ++it)
    delete it.value().doc;
  cache.clear();
  avatars.clear();
}

QString BubbleDelegate::cardFor(const QModelIndex& index) const {
  int type = index.data(ChatMessageModel::TypeRole).toInt();
  bool own = index.data(ChatMessageModel::OwnRole).toBool();
  switch(type) {
  case MT_Broadcast:
    return QStringLiteral("broadcast");
  case MT_File:
  case MT_Folder:
    return QStringLiteral("request");
  case MT_Failed:
  case MT_Join:
  case MT_Leave:
    return QStringLiteral("system");
  default:
    break;
  }
  return own ? QStringLiteral("outgoing") : QStringLiteral("incoming");
}

QColor BubbleDelegate::cardColor(const QString& cardName, bool bg2, bool darkMode) const {
  if(!theme.cards.contains(cardName))
    return darkMode ? QColor(QStringLiteral("#262D36")) : QColor(Qt::white);
  const Theme::Card& card = theme.cards.value(cardName);
  QString ref = bg2 ? (card.bg2.isEmpty() ? card.bg : card.bg2) : card.bg;
  QColor c = theme.resolve(ref, mode, systemDark);
  if(!c.isValid())
    c = theme.resolveToken(QStringLiteral("bubbleIn"), darkMode);
  return c;
}

QFont BubbleDelegate::rowFont(const QModelIndex& index) const {
  static const double sizes[] = {8.25, 9.0, 10.5};
  double themeSize = sizes[qBound(0, display.fontSizeVal, FS_COUNT - 1)];
  QVariant v = index.data(ChatMessageModel::FontRole);
  if(v.isValid()) {
    //  Sender chose a font: honor family and size (like color).
    QFont f = v.value<QFont>();
    if(f.pointSizeF() <= 0)
      f.setPointSizeF(themeSize);
    return f;
  }
  //  Sender did not choose: theme fallback (app default family, local size).
  QFont f;
  f.setPointSizeF(themeSize);
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

QString BubbleDelegate::decodeBody(QString text) const {
  //  Port of lmcMessageLog::decodeMessage/processMessageText (no QTextBrowser).
  if(display.trimMessage)
    text = text.trimmed();
  if(display.allowLinks) {
    text.replace(QRegularExpression(
                   QStringLiteral("((?:(?:https?|ftp|file)://|www\\.|ftp\\.)"
                                  "[-A-Z0-9+&@#/%=~_|$?!:,.]*[A-Z0-9+&@#/%=~_|$])"),
                   QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("<a data-isLink='true' href='\\1'>\\1</a>"));
    text.replace(QStringLiteral("<a data-isLink='true' href='www"),
                 QStringLiteral("<a data-isLink='true' href='http://www"));
    if(display.pathToLink)
      text.replace(QRegularExpression(QStringLiteral("((\\\\\\\\[\\w-]+\\\\[^\\\\/:*?<>|"
                                                     "]+)((?:\\\\[^\\\\/:*?<>|"
                                                     "]+)*\\\\?)$)")),
                   QStringLiteral("<a data-isLink='true' href='file:\\1'>\\1</a>"));
  }
  QString out;
  int pos = 0;
  while(pos < text.length()) {
    int start = text.indexOf(QStringLiteral("<a data-isLink='true'"), pos);
    if(start != -1) {
      QString segment = text.mid(pos, start - pos);
      ChatHelper::makeHtmlSafe(&segment);
      if(display.showSmiley)
        ChatHelper::decodeSmileys(&segment);
      out += segment;
      int end = text.indexOf(QStringLiteral("</a>"), start) + 4;
      out += text.mid(start, end - start);
      pos = end;
    } else {
      QString segment = text.mid(pos);
      ChatHelper::makeHtmlSafe(&segment);
      if(display.showSmiley)
        ChatHelper::decodeSmileys(&segment);
      out += segment;
      break;
    }
  }
  out.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
  return out;
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
  QString kind = (type == MT_Folder) ? tr("folder") : tr("file");
  QString caption = (fileMode == QLatin1String("receive"))
                      ? tr("%1 sends you a %2:").arg(userName, kind)
                      : tr("Sending '%1' to %2.").arg(fileName, userName);
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

QString BubbleDelegate::rowHtml(const QModelIndex& index) const {
  int type = index.data(ChatMessageModel::TypeRole).toInt();
  bool isFile = (type == MT_File || type == MT_Folder);
  QFont font = rowFont(index);
  QColor color = index.data(ChatMessageModel::ColorRole).value<QColor>();
  if(!color.isValid()) {
    //  Per-card text (e.g. light text on a saturated outgoing bubble),
    //  then the global text token. Cards without text keep legacy behavior.
    QString card = cardFor(index);
    QString ref;
    if(theme.cards.contains(card))
      ref = theme.cards.value(card).text;
    if(!ref.isEmpty())
      color = theme.resolve(ref, mode, systemDark);
    if(!color.isValid())
      color = theme.resolveToken(QStringLiteral("text"), theme.isDark(mode, systemDark));
  }
  bool own = index.data(ChatMessageModel::OwnRole).toBool();
  bool fontExplicit = index.data(ChatMessageModel::FontRole).isValid();
  QString style = fontStyleFor(font, color, display.fontSizeVal, own, fontExplicit);
  QString linkColorName =
    theme.resolveToken(QStringLiteral("link"), theme.isDark(mode, systemDark)).name();
  QString body;
  if(isFile) {
    body = fileCardHtml(index);
  } else {
    QString text = index.data(ChatMessageModel::TextRole).toString();
    body = decodeBody(text);
  }
  return QStringLiteral("<div style='%1'>%2</div>").arg(style, body) +
         QStringLiteral("<style>a { color: %1; }</style>").arg(linkColorName);
}

int BubbleDelegate::contentWidth(const QModelIndex& index, int viewportWidth) const {
  QString card = cardFor(index);
  bool grouped = index.data(ChatMessageModel::GroupedRole).toBool();
  int avail = viewportWidth - 2 * kRowMargin;
  if(reservesAvatar(card, grouped))
    avail -= theme.avatar.size + kAvatarGap;
  if(card == QLatin1String("incoming") || card == QLatin1String("outgoing") ||
     card == QLatin1String("request"))
    avail = avail * theme.bubble.maxWidthPct / 100;
  int padH = theme.bubble.padding[1] + theme.bubble.padding[3];
  return qMax(80, avail - padH);
}

QRect BubbleDelegate::rowBubbleRect(const QModelIndex& index, int viewportWidth,
                                    int contentH) const {
  QString card = cardFor(index);
  int bubbleW = contentWidth(index, viewportWidth) + theme.bubble.padding[1] +
                theme.bubble.padding[3];
  int bubbleH = contentH + theme.bubble.padding[0] + theme.bubble.padding[2];
  int x = kRowMargin;
  bool grouped = index.data(ChatMessageModel::GroupedRole).toBool();
  bool hasHeader =
    !grouped && (card == QLatin1String("incoming") || card == QLatin1String("outgoing"));
  int headerH = 0;
  if(hasHeader) {
    QFont hf = rowFont(index);
    if(theme.header.senderBold)
      hf.setBold(true);
    headerH = QFontMetrics(hf).height() + kHeaderGap;
  }
  int avatarW = 0;
  if(reservesAvatar(card, grouped))
    avatarW = theme.avatar.size + kAvatarGap;
  if(card == QLatin1String("outgoing"))
    x = viewportWidth - kRowMargin - bubbleW;
  else if(card == QLatin1String("system"))
    x = (viewportWidth - bubbleW) / 2;
  else
    x = kRowMargin + avatarW;
  return QRect(x, headerH, bubbleW, bubbleH);
}

QString BubbleDelegate::contentRev(const QModelIndex& index) const {
  QString text = index.data(ChatMessageModel::TextRole).toString();
  QString avatar = index.data(ChatMessageModel::AvatarRole).toString();
  QString fileOp = index.data(ChatMessageModel::FileOpRole).toString();
  QString user = index.data(ChatMessageModel::UserNameRole).toString();
  QString time = index.data(ChatMessageModel::TimeRole).toDateTime().toString(Qt::ISODate);
  return QStringLiteral("%1|%2|%3|%4|%5|%6")
    .arg(text.length())
    .arg(qHash(text))
    .arg(qHash(avatar + fileOp + user + time))
    .arg(index.data(ChatMessageModel::GroupedRole).toBool())
    .arg(index.data(ChatMessageModel::OwnRole).toBool())
    .arg(index.data(ChatMessageModel::TypeRole).toInt());
}

QString BubbleDelegate::cacheKey(const QModelIndex& index, int viewportWidth) const {
  qreal dpr = 1.0;
  if(QWidget* w = qobject_cast<QWidget*>(parent()))
    dpr = w->devicePixelRatioF();
  return QStringLiteral("%1|%2|%3|%4|%5")
    .arg(index.row())
    .arg(viewportWidth)
    .arg(int(dpr * 100))
    .arg(themeRev)
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
  e.doc->setHtml(rowHtml(index));
  e.doc->setTextWidth(contentWidth(index, viewportWidth));
  QSizeF docSize = e.doc->size();
  e.bubbleRect = rowBubbleRect(index, viewportWidth, qCeil(docSize.height()));
  e.docRect = QRect(e.bubbleRect.left() + theme.bubble.padding[3],
                    e.bubbleRect.top() + theme.bubble.padding[0],
                    qCeil(docSize.width()), qCeil(docSize.height()));
  e.size = QSize(viewportWidth, e.bubbleRect.bottom() +
                                  (index.data(ChatMessageModel::GroupedRole).toBool()
                                     ? theme.bubble.groupedSpacing
                                     : theme.bubble.spacing));
  cache.insert(key, e);
  return cache.constFind(key).value();
}

bool BubbleDelegate::reservesAvatar(const QString& card, bool grouped) const {
  return card == QLatin1String("incoming") && theme.avatar.size > 0 &&
         (!grouped || theme.avatar.showOnGrouped);
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
  bool darkMode = theme.isDark(mode, systemDark);
  QString card = cardFor(index);
  const CacheEntry& e = entryFor(index, option.rect.width());
  int top = option.rect.top();

  painter->fillRect(option.rect, theme.resolveToken(QStringLiteral("page"), darkMode));

  //  Header (sender + time). Incoming/outgoing only: layout_system/
  //  broadcast/request have no header (stage 1 tree is hardcoded 1:1 with layout_*).
  //  incoming: sender + time; outgoing: time only.
  bool grouped = index.data(ChatMessageModel::GroupedRole).toBool();
  bool hasHeader =
    !grouped && (card == QLatin1String("incoming") || card == QLatin1String("outgoing"));
  if(hasHeader) {
    QFont hf = rowFont(index);
    if(theme.header.senderBold)
      hf.setBold(true);
    painter->setFont(hf);
    painter->setPen(theme.resolveToken(QStringLiteral("secondary"), darkMode));
    QString time = rowTime(index);
    //  layout_incoming: items=[sender, time]; layout_outgoing: items=[time].
    QString header = time;
    if(card == QLatin1String("incoming")) {
      QString sender = index.data(ChatMessageModel::UserNameRole).toString();
      header = time.isEmpty() ? sender : sender + QStringLiteral("  ") + time;
    }
    int hx = e.bubbleRect.left();
    if(card == QLatin1String("outgoing"))
      hx = e.bubbleRect.right() - QFontMetrics(hf).horizontalAdvance(header);
    painter->drawText(hx, top + QFontMetrics(hf).ascent(), header);
  }

  QRect bubble = e.bubbleRect.translated(0, top);
  QRect docRect = e.docRect.translated(0, top);

  if(reservesAvatar(card, grouped)) {
    QString avatarPath = index.data(ChatMessageModel::AvatarRole).toString();
    QPixmap pm = avatarPixmap(avatarPath, theme.avatar.size);
    int ay = bubble.top() + qMax(0, (bubble.height() - theme.avatar.size) / 2);
    painter->drawPixmap(kRowMargin, ay, pm);
  }

  {
    QColor shadow(0, 0, 0);
    shadow.setAlphaF(theme.bubble.shadowOpacity);
    painter->setPen(Qt::NoPen);
    painter->setBrush(shadow);
    QRect shadowRect = bubble.translated(int(theme.bubble.shadowDx), int(theme.bubble.shadowDy));
    painter->drawRoundedRect(shadowRect, theme.bubble.radius[0], theme.bubble.radius[0]);
  }

  QColor bg = cardColor(card, false, darkMode);
  QColor bg2 = cardColor(card, true, darkMode);
  QLinearGradient grad(bubble.topLeft(), bubble.bottomLeft());
  grad.setColorAt(0, bg);
  grad.setColorAt(1, bg2.isValid() ? bg2 : bg);
  painter->setBrush(grad);
  const Theme::Card& cardDef = theme.cards.value(card);
  if(!cardDef.border.isEmpty()) {
    painter->setPen(theme.resolve(cardDef.border, mode, systemDark));
  } else {
    painter->setPen(Qt::NoPen);
  }
  QPainterPath path;
  //  QPainterPath::roundedRect only does uniform rounding; per-corner radius
  //  is an approximation (fine-tune in stage 2).
  path.addRoundedRect(bubble, theme.bubble.radius[0], theme.bubble.radius[0]);
  painter->drawPath(path);

  if(theme.bubble.tail > 0 && cardDef.tail != QLatin1String("none")) {
    int tail = theme.bubble.tail;
    QPolygon tailPoly;
    if(cardDef.tail == QLatin1String("left")) {
      int ty = bubble.top() + bubble.height() / 3;
      tailPoly << QPoint(bubble.left(), ty) << QPoint(bubble.left() - tail, ty + tail / 2)
               << QPoint(bubble.left(), ty + tail);
    } else {
      int ty = bubble.top() + bubble.height() / 3;
      tailPoly << QPoint(bubble.right(), ty) << QPoint(bubble.right() + tail, ty + tail / 2)
               << QPoint(bubble.right(), ty + tail);
    }
    painter->setPen(Qt::NoPen);
    painter->setBrush(bg);
    painter->drawPolygon(tailPoly);
  }

  //  Body: a single QTextDocument chunk. drawContents paints in document
  //  coordinates, so translate the painter to docRect.
  painter->save();
  painter->setClipRect(bubble);
  painter->translate(docRect.topLeft());
  e.doc->drawContents(painter, QRectF(0, 0, docRect.width(), docRect.height()));
  painter->restore();

  if(index.row() == focusRow && !focusHref.isEmpty()) {
    QRect focusRect = anchorRect(focusHref, option, index);
    if(!focusRect.isEmpty()) {
      painter->setPen(QPen(theme.resolveToken(QStringLiteral("link"), darkMode), 2));
      painter->setBrush(Qt::NoBrush);
      painter->drawRoundedRect(focusRect.adjusted(-2, -1, 2, 1), 3, 3);
    }
  }
  painter->restore();
}

QSize BubbleDelegate::sizeHint(const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
  const CacheEntry& e = entryFor(index, option.rect.width());
  return e.size;
}

QString BubbleDelegate::anchorAt(const QPoint& pos, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
  const CacheEntry& e = entryFor(index, option.rect.width());
  QRect docRect = e.docRect.translated(0, option.rect.top());
  if(!docRect.contains(pos))
    return QString();
  QPointF docPos = QPointF(pos - docRect.topLeft());
  QString href = e.doc->documentLayout()->anchorAt(docPos);
  return href;
}

QRect BubbleDelegate::anchorRect(const QString& href, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
  const CacheEntry& e = entryFor(index, option.rect.width());
  QRect docRect = e.docRect.translated(0, option.rect.top());
  if(href.isEmpty() || !e.doc)
    return QRect();
  for(QTextBlock block = e.doc->begin(); block != e.doc->end(); block = block.next()) {
    if(!block.layout())
      continue;
    QRect blockRect = e.doc->documentLayout()->blockBoundingRect(block).toRect();
    for(QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it) {
      QTextFragment frag = it.fragment();
      if(!frag.charFormat().isAnchor() || frag.charFormat().anchorHref() != href)
        continue;
      int start = frag.position() - block.position();
      int end = start + frag.length();
      QTextLayout* layout = block.layout();
      for(int i = 0; i < layout->lineCount(); i++) {
        QTextLine line = layout->lineAt(i);
        int ls = line.textStart();
        int le = ls + line.textLength();
        int s = qMax(start, ls);
        int en = qMin(end, le);
        if(s < en) {
          qreal x1 = line.cursorToX(s);
          qreal x2 = line.cursorToX(en);
          QRect r(docRect.left() + blockRect.left() + int(x1),
                  docRect.top() + blockRect.top() + int(line.y()),
                  int(x2 - x1), int(line.height()));
          return r;
        }
      }
    }
  }
  return QRect();
}

void BubbleDelegate::setFocusAnchor(int row, const QString& href) {
  focusRow = row;
  focusHref = href;
}

void BubbleDelegate::clearFocusAnchor() {
  focusRow = -1;
  focusHref.clear();
}

QStringList BubbleDelegate::anchors(const QModelIndex& index, int viewportWidth) const {
  const CacheEntry& e = entryFor(index, viewportWidth);
  QStringList out;
  if(!e.doc)
    return out;
  for(QTextBlock block = e.doc->begin(); block != e.doc->end(); block = block.next()) {
    for(QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it) {
      QTextFragment frag = it.fragment();
      if(frag.charFormat().isAnchor()) {
        QString href = frag.charFormat().anchorHref();
        if(!href.isEmpty() && !out.contains(href))
          out.append(href);
      }
    }
  }
  return out;
}
