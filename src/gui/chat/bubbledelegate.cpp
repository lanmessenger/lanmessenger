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

//  Layout flag gate: a node renders when its if-flag holds and its
//  ifNot-flag does not. Series position never gates nodes: first and
//  subsequent rows walk different trees. Only compiled structs here,
//  no QJson.
bool nodeVisible(const Theme::LayoutNode& n, bool own, bool file, bool preview) {
  auto on = [&](const QString& flag) {
    if(flag == QLatin1String("isOwn"))
      return own;
    if(flag == QLatin1String("isFile"))
      return file;
    if(flag == QLatin1String("hasPreview"))
      return preview;
    return false;
  };
  if(!n.ifFlag.isEmpty() && !on(n.ifFlag))
    return false;
  if(!n.ifNotFlag.isEmpty() && on(n.ifNotFlag))
    return false;
  return true;
}

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

//  Row role from the message alone: layout name minus the prefix,
//  so "layout_" + role always names the tree to walk.
QString BubbleDelegate::roleFor(const QModelIndex& index) const {
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

double BubbleDelegate::localPointSize() const {
  static const double sizes[] = {8.25, 9.0, 10.5};
  return sizes[qBound(0, display.fontSizeVal, FS_COUNT - 1)];
}

QFont BubbleDelegate::headerNameFont() const {
  //  No index: the message font cannot leak into the header.
  QFont f;
  f.setPointSizeF(localPointSize() * theme.header.senderSize);
  if(theme.header.senderBold)
    f.setBold(true);
  return f;
}

QFont BubbleDelegate::headerTimeFont() const {
  QFont f;
  f.setPointSizeF(localPointSize() * theme.header.timeSize);
  return f;
}

QFont BubbleDelegate::rowFont(const QModelIndex& index) const {
  double themeSize = localPointSize();
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

BubbleDelegate::ResolvedRow BubbleDelegate::resolveRow(const QModelIndex& index) const {
  QString card = roleFor(index);
  bool own = index.data(ChatMessageModel::OwnRole).toBool();
  int type = index.data(ChatMessageModel::TypeRole).toInt();
  bool file = (type == MT_File || type == MT_Folder);
  bool preview = !index.data(ChatMessageModel::PreviewRole).toString().isEmpty();
  ResolvedRow r;
  r.card = card;
  //  Theme defaults first; layout nodes override below. The bg default
  //  blends with the page (unreachable with valid themes: every layout
  //  carries a bubble, but a fully flag-gated row must still paint valid).
  r.text = theme.text;
  r.senderColor = theme.header.senderColor;
  r.timeColor = theme.header.timeColor;
  r.bg.kind = Theme::Bg::Kind::Solid;
  r.bg.solid = theme.page;
  r.avatar = reservesAvatar(card);
  r.header = false;
  if(card == QLatin1String("incoming"))
    r.headerItems = QStringList{QStringLiteral("sender"), QStringLiteral("time")};
  else if(card == QLatin1String("outgoing"))
    r.headerItems = QStringList{QStringLiteral("time")};
  //  In-series rows walk their own tree: no per-node grouped gate.
  QString layoutName = QStringLiteral("layout_") + card;
  if(index.data(ChatMessageModel::GroupedRole).toBool() &&
     (card == QLatin1String("incoming") || card == QLatin1String("outgoing")))
    layoutName += QStringLiteral("_next");
  QVector<Theme::LayoutNode> nodes = theme.compiledLayout(layoutName);
  const Theme::LayoutNode* root = nullptr;
  for(const Theme::LayoutNode& n : nodes) {
    if((n.type != Theme::LayoutNode::Type::HBox && n.type != Theme::LayoutNode::Type::VBox) ||
       !nodeVisible(n, own, file, preview))
      continue;
    root = &n;
    break;
  }
  if(!root)
    return r;
  if(root->type == Theme::LayoutNode::Type::HBox) {
    r.hboxGap = root->gap;
    r.avatar = false;
    const Theme::LayoutNode* column = nullptr;
    for(const Theme::LayoutNode& c : root->children) {
      if(!nodeVisible(c, own, file, preview))
        continue;
      if(c.type == Theme::LayoutNode::Type::Avatar)
        r.avatar = reservesAvatar(card);
      else if(c.type == Theme::LayoutNode::Type::VBox && !column)
        column = &c;
    }
    if(!column) {
      r.header = false;
      r.headerItems.clear();
      return r;
    }
    root = column;
  }
  //  VBox level (root or the hbox column): header + body/fileCard.
  r.vboxGap = root->gap;
  r.header = false;
  r.headerItems.clear();
  for(const Theme::LayoutNode& c : root->children) {
    if(!nodeVisible(c, own, file, preview))
      continue;
    if(c.type == Theme::LayoutNode::Type::Header) {
      r.header = true;
      r.headerItems = c.headerItems;
      if(r.headerItems.isEmpty()) {
        //  Bare header node: card default (incoming shows both).
        if(card == QLatin1String("incoming"))
          r.headerItems = QStringList{QStringLiteral("sender"), QStringLiteral("time")};
        else if(card == QLatin1String("outgoing"))
          r.headerItems = QStringList{QStringLiteral("time")};
      }
      r.senderColor = c.hasSenderColor ? c.senderColor : theme.header.senderColor;
      r.timeColor = c.hasTimeColor ? c.timeColor : theme.header.timeColor;
    } else if(c.type == Theme::LayoutNode::Type::Body ||
              c.type == Theme::LayoutNode::Type::FileCard) {
      r.text = c.hasTextColor ? c.textColor : theme.text;
      r.bg = c.bg;
      for(int i = 0; i < 4; i++)
        r.padding[i] = c.padding[i];
      r.shadowDx = c.shadowDx;
      r.shadowDy = c.shadowDy;
      r.shadowBlur = c.shadowBlur;
      r.shadowOpacity = c.shadowOpacity;
      r.margin = c.margin;
      r.maxWidthPct = c.maxWidthPct;
      r.hasBorder = c.hasBorder;
      r.border = c.border;
      for(int i = 0; i < 4; i++)
        r.corners[i] = c.corners[i];
    }
    //  L-reserved blocks stay ignored like before.
  }
  return r;
}

QString BubbleDelegate::rowHtml(const QModelIndex& index, const ResolvedRow& row) const {
  int type = index.data(ChatMessageModel::TypeRole).toInt();
  bool isFile = (type == MT_File || type == MT_Folder);
  QFont font = rowFont(index);
  QColor color = index.data(ChatMessageModel::ColorRole).value<QColor>();
  if(!color.isValid()) {
    //  Explicit message color wins, then the resolved row text
    //  (layout body/fileCard node or theme default, e.g. light text on
    //  a saturated outgoing bubble).
    color = Theme::pick(row.text, Theme::isDark(mode, systemDark));
  }
  bool own = index.data(ChatMessageModel::OwnRole).toBool();
  bool fontExplicit = index.data(ChatMessageModel::FontRole).isValid();
  QString style = fontStyleFor(font, color, display.fontSizeVal, own, fontExplicit);
  QString linkColorName = Theme::pick(theme.link, Theme::isDark(mode, systemDark)).name();
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

int BubbleDelegate::contentWidth(const QModelIndex& index, int viewportWidth,
                                 const ResolvedRow& row) const {
  Q_UNUSED(index);
  QString card = row.card;
  int avail = viewportWidth;
  if(row.avatar)
    avail -= theme.avatar.size + row.hboxGap;
  if(card == QLatin1String("incoming") || card == QLatin1String("outgoing") ||
     card == QLatin1String("request"))
    avail = avail * row.maxWidthPct / 100;
  int padH = row.padding[1] + row.padding[3];
  return qMax(80, avail - padH);
}

int BubbleDelegate::rowTopMargin(const QModelIndex& index) const {
  //  Per-tree scalar: series-start trees carry the wide gap, _next
  //  trees the dense in-series one.
  return resolveRow(index).margin;
}

void BubbleDelegate::rowCornerRadii(const QModelIndex& index, int out[4]) const {
  //  Corners come from the layout body/fileCard node verbatim: outgoing nodes
  //  carry hand-mirrored values, no code mirror exists.
  ResolvedRow row = resolveRow(index);
  for(int i = 0; i < 4; i++)
    out[i] = row.corners[i];
}

QRect BubbleDelegate::rowBubbleRect(const QModelIndex& index, int viewportWidth, int contentH,
                                    const ResolvedRow& row) const {
  QString card = row.card;
  int bubbleW = contentWidth(index, viewportWidth, row) + row.padding[1] +
                row.padding[3];
  int bubbleH = contentH + row.padding[0] + row.padding[2];
  int x = 0;
  int top = rowTopMargin(index);
  int headerH = 0;
  if(row.header) {
    QFont nf = headerNameFont();
    QFont tf = headerTimeFont();
    headerH = qMax(QFontMetrics(nf).height(), QFontMetrics(tf).height()) + row.vboxGap;
  }
  int avatarW = 0;
  if(row.avatar)
    avatarW = theme.avatar.size + row.hboxGap;
  if(card == QLatin1String("outgoing"))
    x = viewportWidth - bubbleW;
  else if(card == QLatin1String("system"))
    x = (viewportWidth - bubbleW) / 2;
  else
    x = avatarW;
  return QRect(x, top + headerH, bubbleW, bubbleH);
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
  ResolvedRow row = resolveRow(index);
  e.doc->setHtml(rowHtml(index, row));
  e.doc->setTextWidth(contentWidth(index, viewportWidth, row));
  QSizeF docSize = e.doc->size();
  e.bubbleRect = rowBubbleRect(index, viewportWidth, qCeil(docSize.height()), row);
  e.docRect = QRect(e.bubbleRect.left() + row.padding[3],
                    e.bubbleRect.top() + row.padding[0],
                    qCeil(docSize.width()), qCeil(docSize.height()));
  e.topMargin = rowTopMargin(index);
  //  Corners from the already-resolved row: no second tree walk.
  //  rowCornerRadii() stays for tests and external callers.
  for(int i = 0; i < 4; i++)
    e.corner[i] = row.corners[i];
  e.hasAvatar = row.avatar;
  e.hasHeader = row.header;
  e.headerItems = row.headerItems;
  e.senderColor = row.senderColor;
  e.timeColor = row.timeColor;
  e.border = row.border;
  e.hasBorder = row.hasBorder;
  e.bg = row.bg;
  e.shadowDx = row.shadowDx;
  e.shadowDy = row.shadowDy;
  e.shadowBlur = row.shadowBlur;
  e.shadowOpacity = row.shadowOpacity;
  //  +1: QRect::bottom() is the last pixel, size needs one past it.
  e.size = QSize(viewportWidth, e.bubbleRect.bottom() + 1);
  cache.insert(key, e);
  return cache.constFind(key).value();
}

bool BubbleDelegate::reservesAvatar(const QString& card) const {
  //  Avatar space is structural: the node sits in series-start trees
  //  only, so a resolved row either carries it or not.
  return card == QLatin1String("incoming") && theme.avatar.size > 0;
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
  QString card = roleFor(index);
  const CacheEntry& e = entryFor(index, option.rect.width());
  int top = option.rect.top();

  painter->fillRect(option.rect, Theme::pick(theme.page, darkMode));

  //  Header runs come from the layout header node (items + color
  //  overrides); series-start trees carry it, _next trees omit it.
  //  The theme header holds defaults. Name run is senderColor,
  //  time run is timeColor and always regular.
  if(e.hasHeader) {
    QFont nf = headerNameFont();
    QFont tf = headerTimeFont();
    QString time = rowTime(index);
    bool wantSender = e.headerItems.contains(QStringLiteral("sender"));
    bool wantTime = e.headerItems.contains(QStringLiteral("time"));
    QString namePart;
    QString timePart = wantTime ? time : QString();
    bool nameBold = theme.header.senderBold;
    if(card == QLatin1String("incoming")) {
      if(wantSender)
        namePart = index.data(ChatMessageModel::UserNameRole).toString();
      if(!namePart.isEmpty() && !timePart.isEmpty())
        timePart = QStringLiteral("  ") + timePart;
    } else if(card == QLatin1String("outgoing")) {
      if(theme.header.outgoingYouLabel) {
        if(timePart.isEmpty()) {
          namePart = tr("You");
        } else {
          //: Outgoing header prefix, always regular (see header.outgoingYouLabel).
          namePart = tr("You ·");
        }
      }
      nameBold = false;
    }
    if(!nameBold)
      nf.setBold(false);
    QColor nameColor = Theme::pick(e.senderColor, darkMode);
    QColor timeColor = Theme::pick(e.timeColor, darkMode);
    QFontMetrics nfm(nf);
    QFontMetrics tfm(tf);
    int hx = e.bubbleRect.left();
    if(card == QLatin1String("outgoing"))
      hx = e.bubbleRect.right() - nfm.horizontalAdvance(namePart) -
           tfm.horizontalAdvance(timePart);
    int baseline = top + e.topMargin + qMax(nfm.ascent(), tfm.ascent());
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

  QRect bubble = e.bubbleRect.translated(0, top);
  QRect docRect = e.docRect.translated(0, top);

  if(e.hasAvatar) {
    QString avatarPath = index.data(ChatMessageModel::AvatarRole).toString();
    QPixmap pm = avatarPixmap(avatarPath, theme.avatar.size);
    int ay = bubble.top() + qMax(0, (bubble.height() - theme.avatar.size) / 2);
    painter->drawPixmap(0, ay, pm);
  }

  {
    QColor shadow(0, 0, 0);
    shadow.setAlphaF(e.shadowOpacity);
    painter->setPen(Qt::NoPen);
    painter->setBrush(shadow);
    QRect shadowRect = bubble.translated(int(e.shadowDx), int(e.shadowDy));
    painter->drawPath(roundedCorners(shadowRect, e.corner));
  }

  QColor bg, bg2;
  Theme::pickBg(e.bg, darkMode, bg, bg2);
  QLinearGradient grad(bubble.topLeft(), bubble.bottomLeft());
  grad.setColorAt(0, bg);
  grad.setColorAt(1, bg2);
  painter->setBrush(grad);
  if(e.hasBorder) {
    painter->setPen(Theme::pick(e.border, darkMode));
  } else {
    painter->setPen(Qt::NoPen);
  }
  painter->drawPath(roundedCorners(bubble, e.corner));

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
      painter->setPen(QPen(Theme::pick(theme.link, darkMode), 2));
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
