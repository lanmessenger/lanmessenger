// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Unit tests for the ThemeV2 validator (BubbleV2, stage 1).

#include <QtTest>

#include "core/themev2.h"

namespace {

QByteArray validJson() {
  return QByteArray(R"({
    "version": 1,
    "bubble": {"radius": [12, 12, 12, 4], "tail": 8, "shadow": [0, 2, 8, 0.18],
               "padding": [8, 10, 8, 10], "spacing": 6, "groupedSpacing": 2, "maxWidthPct": 75},
    "avatar": {"size": 40, "shape": "circle", "showOnGrouped": false},
    "header": {"senderBold": true, "timeFormat": "HH:mm"},
    "light": {"bubbleIn": "#FFFFFF", "bubbleIn2": "#F1F4F8", "bubbleOut": "#D9F2C7",
              "bubbleOut2": "#C9E8B4", "text": "#1A1D21", "secondary": "#6B7280",
              "link": "#0B5FFF", "border": "#D8DEE5", "page": "#EDEFF2",
              "accent": "#2F9E44", "accentSoft": "#E6F4EA"},
    "dark": {"bubbleIn": "#1F242B", "bubbleIn2": "#262D36", "bubbleOut": "#1D4D2B",
             "bubbleOut2": "#1A4527", "text": "#E8EAED", "secondary": "#9AA0A6",
             "link": "#8AB4F8", "border": "#3A424C", "page": "#131518",
             "accent": "#3FB950", "accentSoft": "#1A2E22"},
    "cards": {
      "incoming": {"bg": "$bubbleIn", "bg2": "$bubbleIn2", "tail": "left", "avatar": true},
      "outgoing": {"bg": "$bubbleOut", "bg2": "$bubbleOut2", "tail": "right", "avatar": false},
      "broadcast": {"bg": "$accentSoft", "bg2": "$accentSoft", "border": "$accent", "tail": "none"},
      "system": {"bg": "transparent", "align": "center", "text": "$secondary"},
      "request": {"bg": "$bubbleIn", "bg2": "$bubbleIn2", "border": "$border", "tail": "left"}
    },
    "layout_incoming": [{"type": "hbox", "gap": 8, "children": [
      {"type": "avatar"},
      {"type": "vbox", "stretch": 1, "children": [
        {"type": "header", "items": ["sender", "time"], "ifNot": "isGrouped"},
        {"type": "bubble", "card": "incoming", "children": [
          {"type": "body"}, {"type": "fileCard", "if": "isFile"}]},
        {"type": "status", "if": "isOwnLast"}]}]}],
    "layout_outgoing": [{"type": "hbox", "gap": 8, "children": [
      {"type": "vbox", "stretch": 1, "children": [
        {"type": "bubble", "card": "outgoing", "children": [{"type": "body"}]}]}]}],
    "layout_system": [{"type": "vbox", "children": [
      {"type": "bubble", "card": "system", "children": [{"type": "body"}]}]}],
    "layout_broadcast": [{"type": "vbox", "children": [
      {"type": "bubble", "card": "broadcast", "children": [{"type": "body"}]}]}],
    "layout_request": [{"type": "vbox", "children": [
      {"type": "bubble", "card": "request", "children": [
        {"type": "body"}, {"type": "fileCard", "if": "isFile"}]}]}]
  })");
}

QByteArray withReplaced(const QByteArray& base, const QByteArray& from, const QByteArray& to) {
  QByteArray out = base;
  out.replace(from, to);
  return out;
}

} // namespace

class TstThemeV2 : public QObject {
  Q_OBJECT

private slots:
  void validTheme();
  void badVersion();
  void hexInCardsRejected();
  void transparentOutsideSystemRejected();
  void unknownTokenRejected();
  void missingLayoutRejected();
  void reservedBlocksAcceptedButIgnored();
  void badFlagRejected();
  void tooDeepRejected();
  void fractionalArrayRejected();
  void resolveTokens();
  void realFileLoads();
};

void TstThemeV2::validTheme() {
  ThemeV2 t = ThemeV2::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.bubble.maxWidthPct, 75);
  QCOMPARE(t.avatar.size, 40);
  QCOMPARE(t.header.timeFormat, QStringLiteral("HH:mm"));
  QVERIFY(t.hasToken(QStringLiteral("bubbleIn")));
  QCOMPARE(t.layouts.size(), 5);
}

void TstThemeV2::badVersion() {
  ThemeV2 t = ThemeV2::loadFromJson(withReplaced(validJson(), "\"version\": 1", "\"version\": 2"));
  QVERIFY(!t.isValid);
}

void TstThemeV2::hexInCardsRejected() {
  ThemeV2 t = ThemeV2::loadFromJson(
    withReplaced(validJson(), "\"bg\": \"$bubbleIn\"", "\"bg\": \"#FFFFFF\""));
  QVERIFY(!t.isValid);
}

void TstThemeV2::transparentOutsideSystemRejected() {
  ThemeV2 t = ThemeV2::loadFromJson(
    withReplaced(validJson(), "\"incoming\": {\"bg\": \"$bubbleIn\"", "\"incoming\": {\"bg\": \"transparent\""));
  QVERIFY(!t.isValid);
}

void TstThemeV2::unknownTokenRejected() {
  ThemeV2 t = ThemeV2::loadFromJson(
    withReplaced(validJson(), "$bubbleIn2", "$nope"));
  QVERIFY(!t.isValid);
}

void TstThemeV2::missingLayoutRejected() {
  QByteArray base = validJson();
  //  Cut out layout_request entirely via marker: easier to corrupt the key name.
  ThemeV2 t = ThemeV2::loadFromJson(withReplaced(base, "layout_request", "layout_nope"));
  QVERIFY(!t.isValid);
}

void TstThemeV2::reservedBlocksAcceptedButIgnored() {
  //  The validator accepts linkPreview/quote/code/buttons (L); the S delegate ignores them.
  QByteArray base = withReplaced(validJson(), "{\"type\": \"body\"}",
                                 "{\"type\": \"quote\"}");
  ThemeV2 t = ThemeV2::loadFromJson(base);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QByteArray base2 = withReplaced(validJson(), "{\"type\": \"body\"}",
                                  "{\"type\": \"linkPreview\"}");
  ThemeV2 t2 = ThemeV2::loadFromJson(base2);
  QVERIFY2(t2.isValid, qPrintable(t2.errorString));
}

void TstThemeV2::badFlagRejected() {
  ThemeV2 t = ThemeV2::loadFromJson(withReplaced(validJson(), "isGrouped", "isFoo"));
  QVERIFY(!t.isValid);
}

void TstThemeV2::tooDeepRejected() {
  //  hbox=1, vbox=2, bubble=3, body=4, + nested body = 5 (>4).
  QByteArray deep = withReplaced(
    validJson(), "{\"type\": \"body\"}",
    "{\"type\": \"bubble\", \"card\": \"incoming\", \"children\": [{\"type\": \"body\"}]}");
  ThemeV2 t = ThemeV2::loadFromJson(deep);
  QVERIFY(!t.isValid);
}

void TstThemeV2::fractionalArrayRejected() {
  //  Pixel arrays are integers: 12.5 would silently truncate via toInt().
  ThemeV2 t = ThemeV2::loadFromJson(
    withReplaced(validJson(), "\"radius\": [12, 12, 12, 4]", "\"radius\": [12, 12, 12, 4.5]"));
  QVERIFY(!t.isValid);
}

void TstThemeV2::resolveTokens() {
  ThemeV2 t = ThemeV2::loadFromJson(validJson());
  QVERIFY(t.isValid);
  QCOMPARE(t.resolve(QStringLiteral("$bubbleIn"), ThemeV2::Mode::Light, false),
           QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(t.resolve(QStringLiteral("$bubbleIn"), ThemeV2::Mode::Dark, false),
           QColor(QStringLiteral("#1F242B")));
  //  System follows systemDark.
  QCOMPARE(t.resolve(QStringLiteral("$text"), ThemeV2::Mode::System, false),
           QColor(QStringLiteral("#1A1D21")));
  QCOMPARE(t.resolve(QStringLiteral("$text"), ThemeV2::Mode::System, true),
           QColor(QStringLiteral("#E8EAED")));
  QCOMPARE(t.resolve(QStringLiteral("transparent"), ThemeV2::Mode::Light, false),
           QColor(Qt::transparent));
}

void TstThemeV2::realFileLoads() {
  QFile f(QString::fromLocal8Bit(LMC_THEMEV2_DIR) + QStringLiteral("/BubbleV2/theme.json"));
  QVERIFY2(f.open(QIODevice::ReadOnly), "cannot open BubbleV2/theme.json");
  ThemeV2 t = ThemeV2::loadFromJson(f.readAll());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
}

QTEST_MAIN(TstThemeV2)
#include "tst_themev2.moc"
