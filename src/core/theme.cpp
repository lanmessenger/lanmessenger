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

const char* kTokens[] = {"bubbleIn", "bubbleIn2", "bubbleOut", "bubbleOut2", "text", "secondary",
                         "link", "border", "page", "accent", "accentSoft", "textInverse"};
const int kTokenCount = 12;

//  Optional tokens (system pill): present in both palettes or in neither.
const char* kOptionalTokens[] = {"pill", "pillBorder"};
const int kOptionalTokenCount = 2;

const char* kCards[] = {"incoming", "outgoing", "broadcast", "system", "request"};
const int kCardCount = 5;

const char* kLayouts[] = {"layout_incoming", "layout_outgoing", "layout_system", "layout_broadcast",
                          "layout_request"};
const int kLayoutCount = 5;

const char* kFlags[] = {"isGrouped", "isFile", "isOwn", "isOwnLast", "hasPreview"};
const int kFlagCount = 5;

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

bool isTokenRef(const QString& s) {
  return s.startsWith(QLatin1Char('$')) && s.length() > 1;
}

QString tokenName(const QString& ref) {
  return ref.mid(1);
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
  static const char* blocks[] = {"hbox", "vbox", "avatar", "header", "bubble", "body",
                                 "linkPreview", "fileCard", "quote", "code", "status", "buttons"};
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

bool validateNode(const QJsonObject& node, int depth, const QMap<QString, QColor>& light,
                  const QMap<QString, QColor>& dark, QString& error) {
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
  for(auto key : {QStringLiteral("if"), QStringLiteral("ifNot")}) {
    if(node.contains(key)) {
      if(!node.value(key).isString() || !isKnownFlag(node.value(key).toString())) {
        error = QStringLiteral("bad %1: %2").arg(key, node.value(key).toString());
        return false;
      }
    }
  }
  if(type == QLatin1String("header") && node.contains(QStringLiteral("items"))) {
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
  if(type == QLatin1String("bubble") && node.contains(QStringLiteral("card"))) {
    QString card = node.value(QStringLiteral("card")).toString();
    bool known = false;
    for(int i = 0; i < kCardCount; i++)
      if(card == QLatin1String(kCards[i]))
        known = true;
    if(!known) {
      error = QStringLiteral("unknown card: %1").arg(card);
      return false;
    }
  }
  if(node.contains(QStringLiteral("children"))) {
    if(!isContainer(type) && type != QLatin1String("bubble") && type != QLatin1String("vbox") &&
       type != QLatin1String("hbox")) {
      //  bubble is also a container (body/fileCard); the rest are leaves.
      if(type != QLatin1String("bubble")) {
        error = QStringLiteral("%1 must not have children").arg(type);
        return false;
      }
    }
    if(!node.value(QStringLiteral("children")).isArray()) {
      error = QStringLiteral("bad children for %1").arg(type);
      return false;
    }
    for(const QJsonValue& c : node.value(QStringLiteral("children")).toArray()) {
      if(!c.isObject()) {
        error = QStringLiteral("bad child of %1").arg(type);
        return false;
      }
      if(!validateNode(c.toObject(), depth + 1, light, dark, error))
        return false;
    }
  }
  Q_UNUSED(light);
  Q_UNUSED(dark);
  return true;
}

} // namespace

QString Theme::defaultJsonText() {
  QFile f(QStringLiteral(":/themes/Bubble/theme.json"));
  if(f.open(QIODevice::ReadOnly))
    return QString::fromUtf8(f.readAll());
  return QString();
}

Theme Theme::builtInDefault() {
  Theme t;
  t.bubble = Bubble();
  t.avatar = Avatar();
  t.header = Header();
  auto fill = [](QMap<QString, QColor>& m, const char* vals[]) {
    for(int i = 0; i < kTokenCount; i++)
      m.insert(QLatin1String(kTokens[i]), QColor(QString::fromLatin1(vals[i])));
  };
  const char* lightVals[] = {"#FFFFFF", "#F1F4F8", "#D9F2C7", "#C9E8B4", "#1A1D21", "#6B7280",
                             "#0B5FFF", "#D8DEE5", "#EDEFF2", "#2F9E44", "#E6F4EA",
                             "#1A1D21"};
  const char* darkVals[] = {"#1F242B", "#262D36", "#1D4D2B", "#1A4527", "#E8EAED", "#9AA0A6",
                            "#8AB4F8", "#3A424C", "#131518", "#3FB950", "#1A2E22",
                            "#E8EAED"};
  static_assert(sizeof(lightVals) / sizeof(lightVals[0]) == kTokenCount,
                "lightVals must cover every token");
  static_assert(sizeof(darkVals) / sizeof(darkVals[0]) == kTokenCount,
                "darkVals must cover every token");
  fill(t.light, lightVals);
  fill(t.dark, darkVals);
  Card in;
  in.bg = QStringLiteral("$bubbleIn");
  in.bg2 = QStringLiteral("$bubbleIn2");
  in.tail = QStringLiteral("left");
  in.avatar = true;
  in.hasAvatar = true;
  t.cards.insert(QStringLiteral("incoming"), in);
  Card out;
  out.bg = QStringLiteral("$bubbleOut");
  out.bg2 = QStringLiteral("$bubbleOut2");
  out.tail = QStringLiteral("right");
  out.avatar = false;
  out.hasAvatar = true;
  t.cards.insert(QStringLiteral("outgoing"), out);
  Card bc;
  bc.bg = QStringLiteral("$accentSoft");
  bc.bg2 = QStringLiteral("$accentSoft");
  bc.border = QStringLiteral("$accent");
  bc.tail = QStringLiteral("none");
  t.cards.insert(QStringLiteral("broadcast"), bc);
  Card sys;
  sys.bg = QStringLiteral("transparent");
  sys.align = QStringLiteral("center");
  sys.text = QStringLiteral("$secondary");
  sys.tail = QStringLiteral("none");
  t.cards.insert(QStringLiteral("system"), sys);
  Card req;
  req.bg = QStringLiteral("$bubbleIn");
  req.bg2 = QStringLiteral("$bubbleIn2");
  req.border = QStringLiteral("$border");
  req.tail = QStringLiteral("left");
  t.cards.insert(QStringLiteral("request"), req);
  //  S renders a hardcoded tree and ignores layouts, but the five names are
  //  part of the contract: layoutFor finds the keys even on the fallback theme.
  for(const char* name : {"layout_incoming", "layout_outgoing", "layout_system",
                          "layout_broadcast", "layout_request"})
    t.layouts.insert(QLatin1String(name), QJsonArray());
  t.isValid = true;
  return t;
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

  //  bubble
  QJsonObject b = root.value(QStringLiteral("bubble")).toObject();
  QString err;
  //  radius/padding are pixel integers; shadow is [dx, dy, blur, opacity]
  //  where only opacity may be fractional (range-checked below).
  if(!checkIntArray(b.value(QStringLiteral("radius")), 4, err, "bubble.radius") ||
     !checkIntArray(b.value(QStringLiteral("padding")), 4, err, "bubble.padding") ||
     !checkDoubleArray(b.value(QStringLiteral("shadow")), 4, err, "bubble.shadow")) {
    t.errorString = err;
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  QJsonArray radius = b.value(QStringLiteral("radius")).toArray();
  QJsonArray padding = b.value(QStringLiteral("padding")).toArray();
  QJsonArray shadow = b.value(QStringLiteral("shadow")).toArray();
  for(int i = 0; i < 4; i++) {
    t.bubble.radius[i] = radius.at(i).toInt();
    t.bubble.padding[i] = padding.at(i).toInt();
    if(t.bubble.radius[i] < 0 || t.bubble.padding[i] < 0) {
      t.errorString = QStringLiteral("bubble.radius/padding must be >= 0");
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
  }
  t.bubble.tail = b.value(QStringLiteral("tail")).toInt(-1);
  t.bubble.margin = b.value(QStringLiteral("margin")).toInt(-1);
  //  Positional margins default to margin only when absent; an explicit
  //  non-number falls back to -1 and invalidates the theme below.
  t.bubble.marginFirstInGroup = t.bubble.margin;
  t.bubble.marginNextInGroup = t.bubble.margin;
  QJsonValue marginFirst = b.value(QStringLiteral("marginFirstInGroup"));
  if(!marginFirst.isUndefined() && !marginFirst.isNull())
    t.bubble.marginFirstInGroup = marginFirst.toInt(-1);
  QJsonValue marginNext = b.value(QStringLiteral("marginNextInGroup"));
  if(!marginNext.isUndefined() && !marginNext.isNull())
    t.bubble.marginNextInGroup = marginNext.toInt(-1);
  t.bubble.shadowDx = shadow.at(0).toDouble();
  t.bubble.shadowDy = shadow.at(1).toDouble();
  t.bubble.shadowBlur = shadow.at(2).toDouble();
  t.bubble.shadowOpacity = shadow.at(3).toDouble();
  if(t.bubble.tail < 0 || t.bubble.margin < 0 || t.bubble.marginFirstInGroup < 0 ||
     t.bubble.marginNextInGroup < 0 || t.bubble.maxWidthPct < 1 || t.bubble.maxWidthPct > 100 ||
     t.bubble.shadowOpacity < 0.0 || t.bubble.shadowOpacity > 1.0) {
    t.errorString = QStringLiteral("bad bubble numbers");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  t.bubble.tailStyle = b.value(QStringLiteral("tailStyle")).toString(QStringLiteral("polygon"));
  if(t.bubble.tailStyle != QLatin1String("polygon") && t.bubble.tailStyle != QLatin1String("rounded") &&
     t.bubble.tailStyle != QLatin1String("none")) {
    t.errorString = QStringLiteral("bad bubble.tailStyle (want polygon|rounded|none)");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }

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

  //  avatar / header
  QJsonObject a = root.value(QStringLiteral("avatar")).toObject();
  t.avatar.size = a.value(QStringLiteral("size")).toInt(-1);
  t.avatar.shape = a.value(QStringLiteral("shape")).toString();
  t.avatar.showOnGrouped = a.value(QStringLiteral("showOnGrouped")).toBool(false);
  QJsonValue avatarGap = a.value(QStringLiteral("gap"));
  if(!avatarGap.isUndefined() && !avatarGap.isNull())
    t.avatar.gap = avatarGap.toInt(-1);
  if(t.avatar.size <= 0 || t.avatar.gap < 0 ||
     (t.avatar.shape != QLatin1String("circle") && t.avatar.shape != QLatin1String("rounded") &&
      t.avatar.shape != QLatin1String("square"))) {
    t.errorString = QStringLiteral("bad avatar");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  QJsonObject h = root.value(QStringLiteral("header")).toObject();
  t.header.senderBold = h.value(QStringLiteral("senderBold")).toBool(true);
  QJsonValue headerGap = h.value(QStringLiteral("gap"));
  if(!headerGap.isUndefined() && !headerGap.isNull())
    t.header.gap = headerGap.toInt(-1);
  if(t.header.gap < 0) {
    t.errorString = QStringLiteral("bad header.gap");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
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
  for(auto key : {QStringLiteral("senderColor"), QStringLiteral("timeColor")}) {
    if(h.contains(key) && !h.value(key).isString()) {
      t.errorString = QStringLiteral("bad header color %1 (want $token)").arg(key);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
  }
  t.header.senderColor =
    h.value(QStringLiteral("senderColor")).toString(QStringLiteral("$secondary"));
  t.header.timeColor = h.value(QStringLiteral("timeColor")).toString(QStringLiteral("$secondary"));

  //  palettes
  for(auto palName : {QStringLiteral("light"), QStringLiteral("dark")}) {
    if(!root.contains(palName) || !root.value(palName).isObject()) {
      t.errorString = QStringLiteral("missing palette %1").arg(palName);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    QJsonObject pal = root.value(palName).toObject();
    for(int i = 0; i < kTokenCount; i++) {
      QString key = QLatin1String(kTokens[i]);
      if(!pal.contains(key) || !pal.value(key).isString() ||
         !isHexColor(pal.value(key).toString())) {
        t.errorString = QStringLiteral("bad token %1.%2").arg(palName, key);
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
      if(palName == QLatin1String("light"))
        t.light.insert(key, QColor(pal.value(key).toString()));
      else
        t.dark.insert(key, QColor(pal.value(key).toString()));
    }
  }

  //  Optional tokens: both palettes or neither, hex-validated like required ones.
  QJsonObject lightPal = root.value(QStringLiteral("light")).toObject();
  QJsonObject darkPal = root.value(QStringLiteral("dark")).toObject();
  for(int i = 0; i < kOptionalTokenCount; i++) {
    QString key = QLatin1String(kOptionalTokens[i]);
    bool hasLight = lightPal.contains(key);
    bool hasDark = darkPal.contains(key);
    if(!hasLight && !hasDark)
      continue;
    if(hasLight != hasDark || !lightPal.value(key).isString() || !darkPal.value(key).isString() ||
       !isHexColor(lightPal.value(key).toString()) ||
       !isHexColor(darkPal.value(key).toString())) {
      t.errorString = QStringLiteral("bad optional token %1").arg(key);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    t.light.insert(key, QColor(lightPal.value(key).toString()));
    t.dark.insert(key, QColor(darkPal.value(key).toString()));
  }

  for(auto key : {QStringLiteral("senderColor"), QStringLiteral("timeColor")}) {
    const QString& v =
      (key == QLatin1String("senderColor")) ? t.header.senderColor : t.header.timeColor;
    if(!isTokenRef(v) || !t.hasToken(tokenName(v))) {
      t.errorString = QStringLiteral("bad header color %1 (want $token)").arg(key);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
  }

  //  cards
  if(!root.contains(QStringLiteral("cards")) || !root.value(QStringLiteral("cards")).isObject()) {
    t.errorString = QStringLiteral("missing cards");
    qCWarning(lmcUi) << "Theme:" << t.errorString;
    return t;
  }
  QJsonObject cards = root.value(QStringLiteral("cards")).toObject();
  for(int i = 0; i < kCardCount; i++) {
    QString name = QLatin1String(kCards[i]);
    if(!cards.contains(name) || !cards.value(name).isObject()) {
      t.errorString = QStringLiteral("missing card %1").arg(name);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    QJsonObject c = cards.value(name).toObject();
    Card card;
    for(auto key : {QStringLiteral("bg"), QStringLiteral("bg2"), QStringLiteral("border"),
                    QStringLiteral("text")}) {
      if(!c.contains(key))
        continue;
      QString v = c.value(key).toString();
      bool isSystemBg = (name == QLatin1String("system") && key == QLatin1String("bg"));
      if(v == QLatin1String("transparent")) {
        if(!isSystemBg) {
          t.errorString = QStringLiteral("transparent only for system.bg");
          qCWarning(lmcUi) << "Theme:" << t.errorString;
          return t;
        }
      } else if(isTokenRef(v)) {
        if(!t.hasToken(tokenName(v))) {
          t.errorString = QStringLiteral("unknown token %1 in card %2").arg(v, name);
          qCWarning(lmcUi) << "Theme:" << t.errorString;
          return t;
        }
      } else {
        t.errorString = QStringLiteral("bad color %1 in card %2 (want $token)").arg(key, name);
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
      if(key == QLatin1String("bg"))
        card.bg = v;
      else if(key == QLatin1String("bg2"))
        card.bg2 = v;
      else if(key == QLatin1String("border"))
        card.border = v;
      else
        card.text = v;
    }
    if(c.contains(QStringLiteral("tail"))) {
      card.tail = c.value(QStringLiteral("tail")).toString();
      if(card.tail != QLatin1String("left") && card.tail != QLatin1String("right") &&
         card.tail != QLatin1String("none")) {
        t.errorString = QStringLiteral("bad tail in card %1").arg(name);
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
    }
    if(c.contains(QStringLiteral("align"))) {
      card.align = c.value(QStringLiteral("align")).toString();
      if(card.align != QLatin1String("left") && card.align != QLatin1String("center") &&
         card.align != QLatin1String("right")) {
        t.errorString = QStringLiteral("bad align in card %1").arg(name);
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
    }
    if(c.contains(QStringLiteral("avatar"))) {
      if(!c.value(QStringLiteral("avatar")).isBool()) {
        t.errorString = QStringLiteral("bad avatar in card %1").arg(name);
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
      card.avatar = c.value(QStringLiteral("avatar")).toBool();
      card.hasAvatar = true;
    }
    t.cards.insert(name, card);
  }

  //  layouts: all 5 are required; validate depth/types/if.
  for(int i = 0; i < kLayoutCount; i++) {
    QString name = QLatin1String(kLayouts[i]);
    if(!root.contains(name) || !root.value(name).isArray()) {
      t.errorString = QStringLiteral("missing %1").arg(name);
      qCWarning(lmcUi) << "Theme:" << t.errorString;
      return t;
    }
    QJsonArray arr = root.value(name).toArray();
    for(const QJsonValue& v : arr) {
      if(!v.isObject()) {
        t.errorString = QStringLiteral("bad node in %1").arg(name);
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
      if(!validateNode(v.toObject(), 1, t.light, t.dark, t.errorString)) {
        t.errorString = name + QStringLiteral(": ") + t.errorString;
        qCWarning(lmcUi) << "Theme:" << t.errorString;
        return t;
      }
    }
    t.layouts.insert(name, arr);
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

bool Theme::hasToken(const QString& token) const {
  return light.contains(token) && dark.contains(token);
}

QColor Theme::resolveToken(const QString& token, bool darkMode) const {
  const auto& pal = darkMode ? dark : light;
  return pal.value(token, QColor());
}

QColor Theme::resolve(const QString& ref, Mode mode, bool systemDark) const {
  if(ref == QLatin1String("transparent"))
    return QColor(Qt::transparent);
  if(!isTokenRef(ref))
    return QColor();
  return resolveToken(tokenName(ref), isDark(mode, systemDark));
}

bool Theme::isDark(Mode mode, bool systemDark) const {
  if(mode == Mode::Dark)
    return true;
  if(mode == Mode::Light)
    return false;
  return systemDark;
}

QJsonArray Theme::layoutFor(const QString& name) const {
  if(layouts.contains(name))
    return layouts.value(name);
  //  Fall back to layout_incoming with a swapped card + a log line.
  qCWarning(lmcUi) << "Theme: missing" << name << "- fallback to layout_incoming";
  return layouts.value(QStringLiteral("layout_incoming"));
}
