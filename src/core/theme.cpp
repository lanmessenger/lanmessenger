// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "theme.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <cmath>

#include "core/lmccategories.h"
#include "core/stdlocation.h"

namespace {

const char* kLayouts[] = {"layout_incoming", "layout_incoming_next", "layout_outgoing",
                          "layout_outgoing_next", "layout_system", "layout_broadcast",
                          "layout_request"};
const int kLayoutCount = 7;

const char* kFlags[] = {"isFile", "isOwn", "hasPreview"};
const int kFlagCount = 3;

bool isHexColor(const QString& s) {
  if(s.length() != 7 && s.length() != 9)
    return false;
  if(!s.startsWith(QLatin1Char('#')))
    return false;
  for(int i = 1; i < s.length(); i++) {
    char c = s.at(i).toLatin1();
    bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    if(!hex)
      return false;
  }
  return true;
}

bool isKnownFlag(const QString& s) {
  for(int i = 0; i < kFlagCount; i++)
    if(s == QLatin1String(kFlags[i]))
      return true;
  return false;
}

bool isKnownLayout(const QString& s) {
  for(int i = 0; i < kLayoutCount; i++)
    if(s == QLatin1String(kLayouts[i]))
      return true;
  return false;
}

//  The S subset is rendered; L-reserved blocks validate but are ignored.
bool isKnownBlock(const QString& s) {
  static const char* blocks[] = {"hbox", "vbox", "avatar", "header", "body", "linkPreview",
                                 "fileCard", "quote", "code", "buttons"};
  for(auto* b : blocks)
    if(s == QLatin1String(b))
      return true;
  return false;
}

bool isContainer(const QString& s) {
  return s == QLatin1String("hbox") || s == QLatin1String("vbox");
}

bool checkDoubleArray(const QJsonValue& v, int n, QString& error, const char* what) {
  if(!v.isArray()) {
    error = QStringLiteral("bad %1: not an array").arg(QLatin1String(what));
    return false;
  }
  QJsonArray a = v.toArray();
  if(a.size() != n) {
    error = QStringLiteral("bad %1: need %2 items").arg(QLatin1String(what)).arg(n);
    return false;
  }
  for(int i = 0; i < n; i++) {
    if(!a.at(i).isDouble()) {
      error = QStringLiteral("bad %1: item %2 not a number").arg(QLatin1String(what)).arg(i);
      return false;
    }
  }
  return true;
}

bool checkIntArray(const QJsonValue& v, int n, QString& error, const char* what) {
  if(!checkDoubleArray(v, n, error, what))
    return false;
  QJsonArray a = v.toArray();
  for(int i = 0; i < n; i++) {
    double d = a.at(i).toDouble();
    if(std::floor(d) != d) {
      error = QStringLiteral("bad %1: item %2 not an integer").arg(QLatin1String(what)).arg(i);
      return false;
    }
  }
  return true;
}

//  Literal color forms: "#hex" (both modes) or ["light", "dark"].
//  Bare "transparent" only where explicitly allowed (body/fileCard bg).
bool parseDualColor(const QJsonValue& v, const char* what, QString& error, Theme::DualColor& out,
                    bool allowTransparent = false) {
  auto bad = [&] {
    error = QStringLiteral("bad %1 (want \"#hex\" or [\"light\", \"dark\"])")
              .arg(QLatin1String(what));
    return false;
  };
  if(v.isString()) {
    QString s = v.toString().trimmed();
    if(allowTransparent && s == QLatin1String("transparent")) {
      out.light = QColor(Qt::transparent);
      out.dark = QColor(Qt::transparent);
      return true;
    }
    if(!isHexColor(s))
      return bad();
    out.light = QColor(s);
    out.dark = QColor(s);
    return true;
  }
  if(!v.isArray())
    return bad();
  QJsonArray a = v.toArray();
  if(a.size() != 2 || !a.at(0).isString() || !a.at(1).isString())
    return bad();
  QString first = a.at(0).toString().trimmed();
  QString second = a.at(1).toString().trimmed();
  if(!isHexColor(first) || !isHexColor(second))
    return bad();
  out.light = QColor(first);
  out.dark = QColor(second);
  return true;
}

//  gradient(#from, #to): exactly two bare hex stops, whitespace tolerated.
bool parseGradient(const QString& s, const char* what, QString& error, QColor& from, QColor& to) {
  QString t = s.trimmed();
  if(!t.startsWith(QLatin1String("gradient(")) || !t.endsWith(QLatin1Char(')'))) {
    error = QStringLiteral("bad %1 (want gradient(#from, #to))").arg(QLatin1String(what));
    return false;
  }
  QString inner = t.mid(9, t.length() - 10);
  const QStringList parts = inner.split(QLatin1Char(','));
  if(parts.size() != 2) {
    error = QStringLiteral("bad %1 (want gradient(#from, #to))").arg(QLatin1String(what));
    return false;
  }
  QString f = parts.at(0).trimmed();
  QString s2 = parts.at(1).trimmed();
  if(!isHexColor(f) || !isHexColor(s2)) {
    error = QStringLiteral("bad %1 (want gradient(#from, #to))").arg(QLatin1String(what));
    return false;
  }
  from = QColor(f);
  to = QColor(s2);
  return true;
}

//  Rect fill: "#hex", "transparent", ["light", "dark"] or
//  ["gradient(#from, #to)", "gradient(#from, #to)"]. A pair mixes
//  either two solids or two gradients, never one of each.
bool parseBg(const QJsonValue& v, const char* what, QString& error, Theme::Bg& out) {
  if(v.isString()) {
    QString s = v.toString().trimmed();
    if(s == QLatin1String("transparent")) {
      out.kind = Theme::Bg::Kind::Transparent;
      return true;
    }
    Theme::DualColor solid;
    if(!parseDualColor(v, what, error, solid))
      return false;
    out.kind = Theme::Bg::Kind::Solid;
    out.solid = solid;
    return true;
  }
  if(!v.isArray() || v.toArray().size() != 2) {
    error = QStringLiteral("bad %1 (want \"#hex\", [\"light\", \"dark\"] or "
                           "[\"gradient(#from, #to)\", \"gradient(#from, #to)\"])")
              .arg(QLatin1String(what));
    return false;
  }
  QJsonArray a = v.toArray();
  if(!a.at(0).isString() || !a.at(1).isString()) {
    error = QStringLiteral("bad %1 (want \"#hex\", [\"light\", \"dark\"] or "
                           "[\"gradient(#from, #to)\", \"gradient(#from, #to)\"])")
              .arg(QLatin1String(what));
    return false;
  }
  QString first = a.at(0).toString().trimmed();
  QString second = a.at(1).toString().trimmed();
  bool firstGrad = first.startsWith(QLatin1String("gradient("));
  bool secondGrad = second.startsWith(QLatin1String("gradient("));
  if(firstGrad != secondGrad) {
    error = QStringLiteral("bad %1 (pair mixes a color and a gradient)").arg(QLatin1String(what));
    return false;
  }
  if(!firstGrad) {
    Theme::DualColor solid;
    if(!parseDualColor(v, what, error, solid))
      return false;
    out.kind = Theme::Bg::Kind::Solid;
    out.solid = solid;
    return true;
  }
  QColor lf, lt, df, dt;
  if(!parseGradient(first, what, error, lf, lt) || !parseGradient(second, what, error, df, dt))
    return false;
  out.kind = Theme::Bg::Kind::Gradient;
  out.lightFrom = lf;
  out.lightTo = lt;
  out.darkFrom = df;
  out.darkTo = dt;
  return true;
}

bool validateNode(const QJsonObject& node, int depth, const QString& layoutName, QString& error) {
  if(depth > 4) {
    error = QStringLiteral("layout too deep (>4)");
    return false;
  }
  if(!node.contains(QStringLiteral("type")) || !node.value(QStringLiteral("type")).isString()) {
    error = QStringLiteral("layout node without type");
    return false;
  }
  QString type = node.value(QStringLiteral("type")).toString();
  if(!isKnownBlock(type)) {
    error = QStringLiteral("unknown block: %1").arg(type);
    return false;
  }
  //  Rows are tailless: no tail triangle is drawn, tail keys are rejected.
  for(auto key : {QStringLiteral("tail"), QStringLiteral("tailSize"),
                  QStringLiteral("tailStyle")}) {
    if(node.contains(key)) {
      error = QStringLiteral("tail is not supported (tailless rows)");
      return false;
    }
  }
  if(isContainer(type)) {
    //  Gap is the only spacing source: containers must carry it (int >= 0).
    QJsonValue gap = node.value(QStringLiteral("gap"));
    if(!gap.isDouble() || std::floor(gap.toDouble()) != gap.toDouble() || gap.toInt() < 0) {
      error = QStringLiteral("bad gap in %1 (want int >= 0)").arg(type);
      return false;
    }
  }
  for(auto key : {QStringLiteral("if"), QStringLiteral("ifNot")}) {
    if(node.contains(key)) {
      if(!node.value(key).isString() || !isKnownFlag(node.value(key).toString())) {
        error = QStringLiteral("bad %1: %2").arg(key, node.value(key).toString());
        return false;
      }
    }
  }
  if(type == QLatin1String("header")) {
    if(node.contains(QStringLiteral("items"))) {
      if(!node.value(QStringLiteral("items")).isArray()) {
        error = QStringLiteral("bad header.items");
        return false;
      }
      for(const QJsonValue& v : node.value(QStringLiteral("items")).toArray()) {
        QString item = v.toString();
        if(item != QLatin1String("sender") && item != QLatin1String("time")) {
          error = QStringLiteral("bad header item: %1").arg(item);
          return false;
        }
      }
    }
    //  Literal header colors: node value or the theme header default.
    for(auto key : {QStringLiteral("senderColor"), QStringLiteral("timeColor")}) {
      if(node.contains(key)) {
        Theme::DualColor unused;
        QByteArray what = QByteArray("header node ") + key.toLatin1();
        if(!parseDualColor(node.value(key), what.constData(), error, unused))
          return false;
      }
    }
    for(auto key :
        {QStringLiteral("bg"), QStringLiteral("textColor"), QStringLiteral("border"),
         QStringLiteral("padding"), QStringLiteral("shadow"), QStringLiteral("margin"),
         QStringLiteral("maxWidthPct")}) {
      if(node.contains(key)) {
        error = QStringLiteral("%1 only on body/fileCard").arg(key);
        return false;
      }
    }
  }
  if(type == QLatin1String("body") || type == QLatin1String("fileCard")) {
    //  Fill is required: a rendered row always has one rect to paint.
    //  One rect per layout: body and fileCard never stack, fileCard
    //  lives in layout_request only.
    if(!node.contains(QStringLiteral("bg"))) {
      error = QStringLiteral("%1 without bg").arg(type);
      return false;
    }
    Theme::Bg unusedBg;
    QByteArray what = type.toLatin1() + QByteArray(" bg");
    if(!parseBg(node.value(QStringLiteral("bg")), what.constData(), error, unusedBg))
      return false;
    for(auto key : {QStringLiteral("textColor"), QStringLiteral("border")}) {
      if(node.contains(key)) {
        Theme::DualColor unused;
        QByteArray keyWhat = type.toLatin1() + QByteArray(" ") + key.toLatin1();
        if(!parseDualColor(node.value(key), keyWhat.constData(), error, unused))
          return false;
      }
    }
    //  Rect metrics, required: pixel padding, shadow, row top gap, width
    //  cap. The gap is per tree now: series-start trees carry the wide
    //  gap, _next trees the dense in-series one, so margin is a scalar.
    QByteArray padWhat = type.toLatin1() + QByteArray(" padding");
    QByteArray shadowWhat = type.toLatin1() + QByteArray(" shadow");
    if(!checkIntArray(node.value(QStringLiteral("padding")), 4, error, padWhat.constData()) ||
       !checkDoubleArray(node.value(QStringLiteral("shadow")), 4, error,
                         shadowWhat.constData()))
      return false;
    QJsonArray padding = node.value(QStringLiteral("padding")).toArray();
    QJsonArray shadow = node.value(QStringLiteral("shadow")).toArray();
    for(int i = 0; i < 4; i++) {
      if(padding.at(i).toInt(-1) < 0) {
        error = QStringLiteral("%1.padding must be >= 0").arg(type);
        return false;
      }
    }
    //  Reject the split-era object shape: a stale {default, next?}
    //  margin reads as a missing scalar below.
    QJsonValue marginValue = node.value(QStringLiteral("margin"));
    if(!marginValue.isDouble() || std::floor(marginValue.toDouble()) != marginValue.toDouble() ||
       marginValue.toInt(-1) < 0) {
      error = QStringLiteral("bad %1 margin (want int >= 0)").arg(type);
      return false;
    }
    //  Hard split: stale pre-split keys fail loudly instead of
    //  being ignored (a stale margin next to a scalar one would
    //  silently keep the scalar).
    for(auto key : {QStringLiteral("marginFirstInGroup"),
                    QStringLiteral("marginNextInGroup"), QStringLiteral("spacing"),
                    QStringLiteral("groupedSpacing")}) {
      if(node.contains(key)) {
        error = QStringLiteral("bad %1: stale key %2").arg(type, key);
        return false;
      }
    }
    int maxWidthPct = node.value(QStringLiteral("maxWidthPct")).toInt(-1);
    if(maxWidthPct < 1 || maxWidthPct > 100 || shadow.at(3).toDouble(-1.0) < 0.0 ||
       shadow.at(3).toDouble(2.0) > 1.0) {
      error = QStringLiteral("bad %1 numbers (want maxWidthPct/shadow opacity)").arg(type);
      return false;
    }
    for(auto key : {QStringLiteral("senderColor"), QStringLiteral("timeColor")}) {
      if(node.contains(key)) {
        error = QStringLiteral("%1 only on header").arg(key);
        return false;
      }
    }
    bool isFileLayout = (layoutName == QLatin1String("layout_request"));
    if(type == QLatin1String("body") && isFileLayout) {
      error = QStringLiteral("body not allowed in layout_request (use fileCard)");
      return false;
    }
    if(type == QLatin1String("fileCard") && !isFileLayout) {
      error = QStringLiteral("fileCard only in layout_request");
      return false;
    }
  }
  //  Color keys on any other node type are a typo: there is nothing to paint.
  if(type != QLatin1String("header") && type != QLatin1String("body") &&
     type != QLatin1String("fileCard")) {
    for(auto key :
        {QStringLiteral("bg"), QStringLiteral("textColor"), QStringLiteral("border"),
         QStringLiteral("padding"), QStringLiteral("shadow"), QStringLiteral("margin"),
         QStringLiteral("maxWidthPct"), QStringLiteral("senderColor"),
         QStringLiteral("timeColor")}) {
      if(node.contains(key)) {
        error = QStringLiteral("%1 only on body/fileCard or header").arg(key);
        return false;
      }
    }
  }
  //  Corners live on body/fileCard nodes only: [TL, TR, BR, BL].
  //  Outgoing nodes carry hand-mirrored values; no code mirror exists.
  //  The split-era {default, next?} object reads as a missing array below.
  if(node.contains(QStringLiteral("corners"))) {
    if(type != QLatin1String("body") && type != QLatin1String("fileCard")) {
      error = QStringLiteral("corners only on body/fileCard");
      return false;
    }
    if(!checkIntArray(node.value(QStringLiteral("corners")), 4, error, "corners"))
      return false;
    QJsonArray corners = node.value(QStringLiteral("corners")).toArray();
    for(int i = 0; i < 4; i++) {
      if(corners.at(i).toInt() < 0) {
        error = QStringLiteral("corners must be >= 0");
        return false;
      }
    }
  }
  if(node.contains(QStringLiteral("children"))) {
    //  Only containers hold children; the rest are leaves.
    if(!isContainer(type)) {
      error = QStringLiteral("%1 must not have children").arg(type);
      return false;
    }
    if(!node.value(QStringLiteral("children")).isArray()) {
      error = QStringLiteral("bad children for %1").arg(type);
      return false;
    }
    QJsonArray kids = node.value(QStringLiteral("children")).toArray();
    for(const QJsonValue& c : kids) {
      if(!c.isObject()) {
        error = QStringLiteral("bad child of %1").arg(type);
        return false;
      }
      if(!validateNode(c.toObject(), depth + 1, layoutName, error))
        return false;
    }
  }
  return true;
}

Theme::LayoutNode compileNode(const QJsonObject& obj) {
  Theme::LayoutNode n;
  QString type = obj.value(QStringLiteral("type")).toString();
  if(type == QLatin1String("hbox"))
    n.type = Theme::LayoutNode::Type::HBox;
  else if(type == QLatin1String("vbox"))
    n.type = Theme::LayoutNode::Type::VBox;
  else if(type == QLatin1String("avatar"))
    n.type = Theme::LayoutNode::Type::Avatar;
  else if(type == QLatin1String("header"))
    n.type = Theme::LayoutNode::Type::Header;
  else if(type == QLatin1String("body"))
    n.type = Theme::LayoutNode::Type::Body;
  else if(type == QLatin1String("fileCard"))
    n.type = Theme::LayoutNode::Type::FileCard;
  n.gap = obj.value(QStringLiteral("gap")).toInt(0);
  for(const QJsonValue& v : obj.value(QStringLiteral("items")).toArray())
    n.headerItems.append(v.toString());
  n.ifFlag = obj.value(QStringLiteral("if")).toString();
  n.ifNotFlag = obj.value(QStringLiteral("ifNot")).toString();
  //  Validator guarantees the shapes; absent optionals keep has-flags false.
  if(obj.contains(QStringLiteral("senderColor"))) {
    QString dummy;
    parseDualColor(obj.value(QStringLiteral("senderColor")), "header node senderColor", dummy,
                   n.senderColor);
    n.hasSenderColor = true;
  }
  if(obj.contains(QStringLiteral("timeColor"))) {
    QString dummy;
    parseDualColor(obj.value(QStringLiteral("timeColor")), "header node timeColor", dummy,
                   n.timeColor);
    n.hasTimeColor = true;
  }
  if(obj.contains(QStringLiteral("textColor"))) {
    QString dummy;
    parseDualColor(obj.value(QStringLiteral("textColor")), "body/fileCard textColor", dummy,
                   n.textColor);
    n.hasTextColor = true;
  }
  if(obj.contains(QStringLiteral("border"))) {
    QString dummy;
    parseDualColor(obj.value(QStringLiteral("border")), "body/fileCard border", dummy, n.border);
    n.hasBorder = true;
  }
  if(obj.contains(QStringLiteral("bg"))) {
    QString dummy;
    parseBg(obj.value(QStringLiteral("bg")), "body/fileCard bg", dummy, n.bg);
  }
  QJsonArray pad = obj.value(QStringLiteral("padding")).toArray();
  for(int i = 0; i < 4; i++)
    n.padding[i] = pad.at(i).toInt();
  QJsonArray sh = obj.value(QStringLiteral("shadow")).toArray();
  n.shadowDx = sh.at(0).toDouble();
  n.shadowDy = sh.at(1).toDouble();
  n.shadowBlur = sh.at(2).toDouble();
  n.shadowOpacity = sh.at(3).toDouble();
  //  Validator guarantees ints; margin is a per-tree scalar.
  n.margin = obj.value(QStringLiteral("margin")).toInt(0);
  n.maxWidthPct = obj.value(QStringLiteral("maxWidthPct")).toInt(75);
  //  Validator guarantees the shape; a node without corners keeps
  //  the built-in default.
  QJsonArray corners = obj.value(QStringLiteral("corners")).toArray();
  if(obj.contains(QStringLiteral("corners")) && !corners.isEmpty()) {
    n.hasCorners = true;
    for(int i = 0; i < 4; i++)
      n.corners[i] = corners.at(i).toInt();
  }
  for(const QJsonValue& c : obj.value(QStringLiteral("children")).toArray()) {
    if(c.isObject())
      n.children.append(compileNode(c.toObject()));
  }
  return n;
}

int treeContentCount(const Theme::LayoutNode& n) {
  int count = (n.type == Theme::LayoutNode::Type::Body ||
               n.type == Theme::LayoutNode::Type::FileCard)
                ? 1
                : 0;
  for(const auto& c : n.children)
    count += treeContentCount(c);
  return count;
}

} // namespace

QString Theme::defaultJsonText() {
  QFile f(QStringLiteral(":/themes/Bubble/theme.json"));
  if(f.open(QIODevice::ReadOnly))
    return QString::fromUtf8(f.readAll());
  return QString();
}

//  Last-resort theme: no resource dependency, minimal but complete.
//  Only runs when even the compiled-in Bubble resource is unreadable.
const char kFallbackThemeJson[] = R"JSON({
  "version": 1,
  "avatar": {"size": 40, "shape": "circle"},
  "header": {
    "senderBold": true, "timeFormat": "HH:mm", "outgoingYouLabel": false,
    "senderColor": ["#1A1D21", "#E8EAED"], "timeColor": ["#6B7280", "#9AA0A6"]
  },
  "text": ["#1A1D21", "#E8EAED"],
  "link": ["#0B5FFF", "#8AB4F8"],
  "page": ["#EDEFF2", "#131518"],
  "layout_incoming": [
    {"type": "hbox", "gap": 8, "children": [
      {"type": "avatar"},
      {"type": "vbox", "gap": 2, "stretch": 1, "children": [
        {"type": "header", "items": ["sender", "time"]},
        {"type": "body", "bg": ["gradient(#FFFFFF, #F1F4F8)",
                                "gradient(#1F242B, #262D36)"],
         "padding": [8, 10, 8, 10],
         "shadow": [0, 2, 8, 0.18], "margin": 6, "maxWidthPct": 75}]}]}
  ],
  "layout_incoming_next": [
    {"type": "hbox", "gap": 8, "children": [
      {"type": "vbox", "gap": 2, "stretch": 1, "children": [
        {"type": "body", "bg": ["gradient(#FFFFFF, #F1F4F8)",
                                "gradient(#1F242B, #262D36)"],
         "padding": [8, 10, 8, 10],
         "shadow": [0, 2, 8, 0.18], "margin": 2, "maxWidthPct": 75}]}]}
  ],
  "layout_outgoing": [
    {"type": "hbox", "gap": 8, "children": [
      {"type": "vbox", "gap": 2, "stretch": 1, "children": [
        {"type": "header", "items": ["time"]},
        {"type": "body", "bg": ["gradient(#D9F2C7, #C9E8B4)",
                                "gradient(#1D4D2B, #1A4527)"],
         "textColor": ["#1A1D21", "#E8EAED"],
         "padding": [8, 10, 8, 10],
         "shadow": [0, 2, 8, 0.18], "margin": 6, "maxWidthPct": 75}]}]}
  ],
  "layout_outgoing_next": [
    {"type": "hbox", "gap": 8, "children": [
      {"type": "vbox", "gap": 2, "stretch": 1, "children": [
        {"type": "body", "bg": ["gradient(#D9F2C7, #C9E8B4)",
                                "gradient(#1D4D2B, #1A4527)"],
         "textColor": ["#1A1D21", "#E8EAED"],
         "padding": [8, 10, 8, 10],
         "shadow": [0, 2, 8, 0.18], "margin": 2, "maxWidthPct": 75}]}]}
  ],
  "layout_system": [
    {"type": "vbox", "gap": 0, "children": [
      {"type": "body", "bg": "transparent",
       "textColor": ["#6B7280", "#9AA0A6"],
       "padding": [8, 10, 8, 10],
       "shadow": [0, 2, 8, 0.18], "margin": 6, "maxWidthPct": 75}]}
  ],
  "layout_broadcast": [
    {"type": "vbox", "gap": 0, "children": [
      {"type": "body", "bg": ["#E6F4EA", "#1A2E22"],
       "border": ["#2F9E44", "#3FB950"],
       "padding": [8, 10, 8, 10],
       "shadow": [0, 2, 8, 0.18], "margin": 6, "maxWidthPct": 75}]}
  ],
  "layout_request": [
    {"type": "vbox", "gap": 0, "children": [
      {"type": "fileCard", "bg": ["gradient(#FFFFFF, #F1F4F8)",
                                  "gradient(#1F242B, #262D36)"],
       "border": ["#D8DEE5", "#3A424C"],
       "padding": [8, 10, 8, 10],
       "shadow": [0, 2, 8, 0.18], "margin": 6, "maxWidthPct": 75}]}
  ]
})JSON";

QByteArray Theme::fallbackJsonText() {
  return QByteArray(kFallbackThemeJson);
}

Theme Theme::builtInDefault() {
  //  Primary fallback: the compiled-in Bubble theme, same file the
  //  theme list shows first.
  Theme t = loadFromJson(defaultJsonText().toUtf8());
  if(t.isValid)
    return t;
  qCWarning(lmcUi) << "Theme: built-in resource broken, using minimal fallback";
  return loadFromJson(fallbackJsonText());
}

Theme Theme::loadFromJson(const QByteArray& data) {
  Theme t;
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if(parseError.error != QJsonParseError::NoError || !doc.isObject()) {
    t.errorString = QStringLiteral("bad json: %1").arg(parseError.errorString());
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  QJsonObject root = doc.object();
  if(!root.contains(QStringLiteral("version")) || root.value(QStringLiteral("version")).toInt(-1) != 1) {
    t.errorString = QStringLiteral("bad version (want 1)");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }

  QString err;

  //  view padding: optional root key, defaults keep the current look.
  QJsonValue viewPadding = root.value(QStringLiteral("viewPadding"));
  if(!viewPadding.isUndefined() && !viewPadding.isNull()) {
    if(!checkIntArray(viewPadding, 4, err, "viewPadding")) {
      t.errorString = err;
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    QJsonArray vp = viewPadding.toArray();
    for(int i = 0; i < 4; i++) {
      t.view.padding[i] = vp.at(i).toInt();
      if(t.view.padding[i] < 0) {
        t.errorString = QStringLiteral("viewPadding must be >= 0");
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
    }
  }

  //  avatar / header (gaps live on layout containers now; stale
  //  theme-level "gap" keys are ignored, not rejected).
  //  avatar: size/shape required; showOnGrouped is gone (avatar space
  //  is structural now: the node sits in series-start trees only).
  QJsonObject a = root.value(QStringLiteral("avatar")).toObject();
  if(a.contains(QStringLiteral("showOnGrouped"))) {
    t.errorString = QStringLiteral("showOnGrouped removed (avatar is structural)");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  t.avatar.size = a.value(QStringLiteral("size")).toInt(-1);
  t.avatar.shape = a.value(QStringLiteral("shape")).toString();
  if(t.avatar.size <= 0 ||
     (t.avatar.shape != QLatin1String("circle") && t.avatar.shape != QLatin1String("rounded") &&
      t.avatar.shape != QLatin1String("square"))) {
    t.errorString = QStringLiteral("bad avatar");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  QJsonObject h = root.value(QStringLiteral("header")).toObject();
  t.header.senderBold = h.value(QStringLiteral("senderBold")).toBool(true);
  t.header.timeFormat = h.value(QStringLiteral("timeFormat")).toString();
  if(t.header.timeFormat.isEmpty()) {
    t.errorString = QStringLiteral("bad header.timeFormat");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  //  The label text itself is not theme data: when true, the delegate renders
  //  the translated "You" prefix (tr() in BubbleDelegate, .ts/.qm workflow).
  t.header.outgoingYouLabel = h.value(QStringLiteral("outgoingYouLabel")).toBool(false);
  for(auto key : {QStringLiteral("senderSize"), QStringLiteral("timeSize")}) {
    if(h.contains(key) && !h.value(key).isDouble()) {
      t.errorString = QStringLiteral("bad header size %1 (want number)").arg(key);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
  }
  t.header.senderSize = h.value(QStringLiteral("senderSize")).toDouble(1.0);
  t.header.timeSize = h.value(QStringLiteral("timeSize")).toDouble(1.0);
  if(t.header.senderSize < 0.5 || t.header.senderSize > 2.0 || t.header.timeSize < 0.5 ||
     t.header.timeSize > 2.0) {
    t.errorString = QStringLiteral("bad header size (want 0.5..2.0)");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  //  Theme-level header colors are defaults; layout header nodes override.
  //  Required like timeFormat: there is no sane hardcoded default.
  for(auto key : {QStringLiteral("senderColor"), QStringLiteral("timeColor")}) {
    QByteArray what = QByteArray("header.") + key.toLatin1();
    Theme::DualColor c;
    if(!h.contains(key) || !parseDualColor(h.value(key), what.constData(), err, c)) {
      t.errorString = err.isEmpty() ? QStringLiteral("missing header.%1").arg(key) : err;
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    if(key == QLatin1String("senderColor"))
      t.header.senderColor = c;
    else
      t.header.timeColor = c;
  }

  //  Single-use global colors: default body text, links, view fill.
  for(auto key : {QStringLiteral("text"), QStringLiteral("link"), QStringLiteral("page")}) {
    QByteArray what = key.toLatin1();
    Theme::DualColor c;
    if(!root.contains(key) || !parseDualColor(root.value(key), what.constData(), err, c)) {
      t.errorString = err.isEmpty() ? QStringLiteral("missing %1").arg(key) : err;
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    if(key == QLatin1String("text"))
      t.text = c;
    else if(key == QLatin1String("link"))
      t.link = c;
    else
      t.page = c;
  }

  //  layouts: all 7 are required, non-empty, each holding exactly
  //  one rect (the delegate always paints one); validate depth/types/if.
  for(int i = 0; i < kLayoutCount; i++) {
    QString name = QLatin1String(kLayouts[i]);
    if(!root.contains(name) || !root.value(name).isArray() ||
       root.value(name).toArray().isEmpty()) {
      t.errorString = QStringLiteral("missing %1").arg(name);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    QJsonArray arr = root.value(name).toArray();
    QVector<Theme::LayoutNode> done;
    int content = 0;
    for(const QJsonValue& v : arr) {
      if(!v.isObject()) {
        t.errorString = QStringLiteral("bad node in %1").arg(name);
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
      if(!validateNode(v.toObject(), 1, name, t.errorString)) {
        t.errorString = name + QStringLiteral(": ") + t.errorString;
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
      Theme::LayoutNode n = compileNode(v.toObject());
      content += treeContentCount(n);
      done.append(n);
    }
    if(content == 0) {
      t.errorString = QStringLiteral("%1 without body/fileCard").arg(name);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    if(content > 1) {
      t.errorString = QStringLiteral("%1 holds one body/fileCard").arg(name);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    t.layouts.insert(name, arr);
    t.compiled.insert(name, done);
  }

  t.isValid = true;
  return t;
}

Theme Theme::loadFromFile(const QString& filePath) {
  QFile f(filePath);
  if(!f.open(QIODevice::ReadOnly)) {
    Theme t = builtInDefault();
    t.isValid = false;
    t.errorString = QStringLiteral("cannot open %1").arg(filePath);
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  Theme t = loadFromJson(f.readAll());
  if(!t.isValid) {
    Theme fallback = builtInDefault();
    fallback.isValid = false;
    fallback.errorString = t.errorString;
    return fallback;
  }
  return t;
}

namespace {

//  Name -> theme.json. Built-in qrc first, user dir last (wins on clash).
//  Broken files are skipped with a log line, never fatal.
QMap<QString, QString> scanThemeFiles() {
  QMap<QString, QString> found;
  auto consider = [&](const QString& filePath) {
    QString name = QFileInfo(filePath).dir().dirName();
    if(name.isEmpty()) {
      qCWarning(lmcUi) << "Theme: skipping nameless" << filePath;
      return;
    }
    Theme t = Theme::loadFromFile(filePath);
    if(!t.isValid) {
      qCWarning(lmcUi) << "Theme: skipping" << filePath << t.errorString;
      return;
    }
    found.insert(name, filePath);
  };
  QString builtIn = QStringLiteral(":/themes/Bubble/theme.json");
  if(QFile::exists(builtIn))
    consider(builtIn);
  else
    qCWarning(lmcUi) << "Theme: missing built-in" << builtIn;
  //  Further built-in themes ship in the same qrc prefix; the user dir wins.
  QDir builtInDir(QStringLiteral(":/themes"));
  const QStringList builtInSubs = builtInDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  for(const QString& sub : builtInSubs) {
    QString filePath = builtInDir.absoluteFilePath(sub + QStringLiteral("/theme.json"));
    if(filePath == builtIn)
      continue;
    if(QFile::exists(filePath))
      consider(filePath);
  }
  QDir userDir(StdLocation::userThemeDir());
  const QStringList subdirs = userDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
  for(const QString& sub : subdirs) {
    QString filePath = userDir.absoluteFilePath(sub + QStringLiteral("/theme.json"));
    if(QFile::exists(filePath))
      consider(filePath);
  }
  return found;
}

} // namespace

QStringList Theme::availableThemes() {
  return scanThemeFiles().keys();
}

QString Theme::themeFileFor(const QString& name) {
  return scanThemeFiles().value(name);
}

QColor Theme::pick(const DualColor& c, bool darkMode) {
  return darkMode ? c.dark : c.light;
}

void Theme::pickBg(const Bg& bg, bool darkMode, QColor& first, QColor& second) {
  if(bg.kind == Bg::Kind::Transparent) {
    first = QColor(Qt::transparent);
    second = QColor(Qt::transparent);
    return;
  }
  if(bg.kind == Bg::Kind::Gradient) {
    if(darkMode) {
      first = bg.darkFrom;
      second = bg.darkTo;
    } else {
      first = bg.lightFrom;
      second = bg.lightTo;
    }
    return;
  }
  first = pick(bg.solid, darkMode);
  second = first;
}

bool Theme::isDark(Mode mode, bool systemDark) {
  if(mode == Mode::Dark)
    return true;
  if(mode == Mode::Light)
    return false;
  return systemDark;
}

QVector<Theme::LayoutNode> Theme::compiledLayout(const QString& name) const {
  return compiled.value(name);
}

QJsonArray Theme::layoutFor(const QString& name) const {
  if(layouts.contains(name))
    return layouts.value(name);
  //  Fall back to layout_incoming + a log line (kept for L).
  qCWarning(lmcUi) << "Theme: missing" << name << "- fallback to layout_incoming";
  return layouts.value(QStringLiteral("layout_incoming"));
}
