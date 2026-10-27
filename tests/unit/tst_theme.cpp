// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Unit tests for the Theme validator (rect metrics, literal node colors).

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include "core/stdlocation.h"
#include "core/theme.h"

namespace {

QByteArray validJson() {
  return QByteArray(R"JSON({
    "version": 1,
    "avatar": {"size": 40, "shape": "circle"},
    "header": {"senderWeight": 600, "timeFormat": "HH:mm",
               "senderColor": ["#6B7280", "#9AA0A6"],
               "timeColor": ["#6B7280", "#9AA0A6"]},
    "text": ["#1A1D21", "#E8EAED"],
    "link": ["#0B5FFF", "#8AB4F8"],
    "page": ["#EDEFF2", "#131518"],
    "layout_incoming": {"type": "hbox", "gap": 8, "children": [
      {"type": "avatar"},
      {"type": "vbox", "gap": 2, "stretch": 1, "children": [
        {"type": "header", "items": ["sender", "time"], "senderColor": ["#1A1D21", "#E8EAED"]},
        {"type": "body", "corners": [12, 12, 12, 4], "bg": ["gradient(#FFFFFF, #F1F4F8)", "gradient(#1F242B, #262D36)"], "padding": [8, 10, 8, 10], "shadow": [0, 2, 0.18], "marginTop": 6, "maxWidthPct": 75}]}]},
    "layout_incoming_next": {"type": "vbox", "gap": 0, "children": [
      {"type": "body", "corners": [4, 12, 12, 4], "bg": ["gradient(#FFFFFF, #F1F4F8)", "gradient(#1F242B, #262D36)"], "padding": [8, 10, 8, 10], "shadow": [0, 2, 0.18], "marginTop": 2, "maxWidthPct": 75}]},
    "layout_outgoing": {"type": "vbox", "gap": 2, "children": [
      {"type": "header", "items": ["time"]},
      {"type": "body", "corners": [12, 12, 4, 12], "bg": ["gradient(#D9F2C7, #C9E8B4)", "gradient(#1D4D2B, #1A4527)"], "textColor": "#1A1D21", "padding": [8, 10, 8, 10], "shadow": [0, 2, 0.18], "marginTop": 6, "maxWidthPct": 75}]},
    "layout_outgoing_next": {"type": "vbox", "gap": 0, "children": [
      {"type": "body", "corners": [12, 4, 4, 12], "bg": ["gradient(#D9F2C7, #C9E8B4)", "gradient(#1D4D2B, #1A4527)"], "textColor": "#1A1D21", "padding": [8, 10, 8, 10], "shadow": [0, 2, 0.18], "marginTop": 2, "maxWidthPct": 75}]},
    "layout_system": {"type": "vbox", "gap": 0, "children": [
      {"type": "body", "bg": "transparent", "textColor": ["#6B7280", "#9AA0A6"], "padding": [8, 10, 8, 10], "shadow": [0, 2, 0.18], "marginTop": 6, "maxWidthPct": 75}]},
    "layout_broadcast": {"type": "vbox", "gap": 0, "children": [
      {"type": "body", "bg": ["#E6F4EA", "#1A2E22"], "border": ["#2F9E44", "#3FB950"], "padding": [8, 10, 8, 10], "shadow": [0, 2, 0.18], "marginTop": 6, "maxWidthPct": 75}]},
    "layout_request": {"type": "vbox", "gap": 0, "children": [
      {"type": "fileCard", "bg": ["gradient(#FFFFFF, #F1F4F8)", "gradient(#1F242B, #262D36)"], "border": ["#D8DEE5", "#3A424C"], "padding": [8, 10, 8, 10], "shadow": [0, 2, 0.18], "marginTop": 6, "maxWidthPct": 75}]}
  })JSON");
}

QByteArray withReplaced(const QByteArray& base, const QByteArray& from, const QByteArray& to) {
  QByteArray out = base;
  out.replace(from, to);
  return out;
}

Theme::LayoutNode incomingBody(const Theme& t) {
  return t.compiledLayout(QStringLiteral("layout_incoming")).children.at(1).children.at(1);
}

Theme::LayoutNode incomingNextBody(const Theme& t) {
  return t.compiledLayout(QStringLiteral("layout_incoming_next")).children.at(0);
}

} // namespace

class TstTheme : public QObject {
  Q_OBJECT

private slots:
  void validTheme();
  void badVersion();
  void optionalKeysDefaulted();
  void outgoingYouLabelParsed();
  void outgoingYouLabelExplicitFalse();
  void headerTypographyDefaulted();
  void headerTypographyParsed();
  void badHeaderSizeRejected();
  void badHeaderWeightRejected();
  void badHeaderColorRejected();
  void headerColorsRequired();
  void globalColorsParsed();
  void missingGlobalColorRejected();
  void badGlobalColorRejected();
  void rectMarginTopParsed();
  void badMarginTopRejected();
  void splitEraShapesRejected();
  void cornerRadiiParsed();
  void cornersAbsentDefaulted();
  void badCornerRadiiRejected();
  void cornersOnNonRectRejected();
  void viewChromeDefaulted();
  void viewChromeParsed();
  void badViewChromeRejected();
  void containerGapRequired();
  void containerGapParsed();
  void badContainerGapRejected();
  void staleThemeGapKeysIgnored();
  void fileCardOutsideRequestRejected();
  void bodyInRequestRejected();
  void twoRectsInLayoutRejected();
  void requestFileCardAccepted();
  void bareHexBothModes();
  void pairParsed();
  void gradientParsed();
  void solidKindsParsed();
  void transparentAccepted();
  void transparentOutsideBgRejected();
  void mixedPairRejected();
  void badGradientRejected();
  void gradientWhitespaceAccepted();
  void gradientOutsideBgRejected();
  void missingBgDefaulted();
  void missingMaxWidthPctDefaulted();
  void rectMetricsParsed();
  void rectMetricsOptionalDefaulted();
  void tailRejected();
  void colorKeyOnWrongNodeRejected();
  void dollarRefsRejected();
  void layoutWithoutRectRejected();
  void missingLayoutRejected();
  void builtInDefaultValid();
  void pickHelpers();
  void shippedMeshBlueSpotCheck();
  void availableThemesListsUserAndSkipsBroken();
  void realFileLoads();
};

void TstTheme::validTheme() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(incomingBody(t).maxWidthPct, 75);
  QCOMPARE(t.avatar.size, 40);
  QCOMPARE(t.header.timeFormat, QStringLiteral("HH:mm"));
  QCOMPARE(t.layouts.size(), 7);
  QCOMPARE(t.compiled.size(), 7);
}

void TstTheme::badVersion() {
  Theme t = Theme::loadFromJson(withReplaced(validJson(), "\"version\": 1", "\"version\": 2"));
  QVERIFY(!t.isValid);
}

void TstTheme::optionalKeysDefaulted() {
  //  No outgoing label, unit sizes, no node overrides.
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY(t.isValid);
  QVERIFY(!t.header.outgoingYouLabel);
  QCOMPARE(t.header.senderSize, 1.0);
  QCOMPARE(t.header.timeSize, 1.0);
  const Theme::LayoutNode& outgoing = t.compiledLayout(QStringLiteral("layout_outgoing"));
  const Theme::LayoutNode& header = outgoing.children.at(0);
  QVERIFY(!header.hasSenderColor);
  QVERIFY(!header.hasTimeColor);
}

void TstTheme::outgoingYouLabelParsed() {
  QByteArray labeled = withReplaced(validJson(), "\"senderWeight\": 600",
                                    "\"senderWeight\": 600, \"outgoingYouLabel\": true");
  Theme t = Theme::loadFromJson(labeled);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QVERIFY(t.header.outgoingYouLabel);
}

void TstTheme::outgoingYouLabelExplicitFalse() {
  QByteArray base = withReplaced(validJson(), "\"senderWeight\": 600",
                                 "\"senderWeight\": 600, \"outgoingYouLabel\": false");
  Theme t = Theme::loadFromJson(base);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QVERIFY(!t.header.outgoingYouLabel);
}

void TstTheme::headerTypographyDefaulted() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY(t.isValid);
  QCOMPARE(t.header.senderWeight, 600);
  QCOMPARE(t.header.senderSize, 1.0);
  QCOMPARE(t.header.timeSize, 1.0);
  QCOMPARE(t.header.senderColor.light, QColor(QStringLiteral("#6B7280")));
  QCOMPARE(t.header.senderColor.dark, QColor(QStringLiteral("#9AA0A6")));
  QCOMPARE(t.header.timeColor.light, QColor(QStringLiteral("#6B7280")));
  QCOMPARE(t.header.timeColor.dark, QColor(QStringLiteral("#9AA0A6")));
}

void TstTheme::headerTypographyParsed() {
  QByteArray typed = withReplaced(validJson(), "\"senderWeight\": 600", "\"senderWeight\": 400");
  typed = withReplaced(typed, "\"timeFormat\": \"HH:mm\"",
                       "\"timeFormat\": \"HH:mm\", \"senderSize\": 1.25, \"timeSize\": 0.75");
  typed = withReplaced(typed, "\"senderColor\": [\"#6B7280\", \"#9AA0A6\"],\n"
                              "               \"timeColor\": [\"#6B7280\", \"#9AA0A6\"]",
                       "\"senderColor\": \"#1A1D21\",\n"
                       "               \"timeColor\": [\"#0B5FFF\", \"#8AB4F8\"]");
  Theme t = Theme::loadFromJson(typed);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.header.senderWeight, 400);
  QCOMPARE(t.header.senderSize, 1.25);
  QCOMPARE(t.header.timeSize, 0.75);
  //  Bare hex covers both modes; a pair splits them.
  QCOMPARE(t.header.senderColor.light, QColor(QStringLiteral("#1A1D21")));
  QCOMPARE(t.header.senderColor.dark, QColor(QStringLiteral("#1A1D21")));
  QCOMPARE(t.header.timeColor.light, QColor(QStringLiteral("#0B5FFF")));
  QCOMPARE(t.header.timeColor.dark, QColor(QStringLiteral("#8AB4F8")));
}

void TstTheme::badHeaderSizeRejected() {
  for(const char* bad : {"0.1", "2.5", "\"big\"", "true"}) {
    QByteArray sized =
      withReplaced(validJson(), "\"timeFormat\": \"HH:mm\"",
                   QByteArray("\"timeFormat\": \"HH:mm\", \"senderSize\": ") + bad);
    Theme t = Theme::loadFromJson(sized);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::badHeaderWeightRejected() {
  //  Weight is an int 100..900: out of range, non-numeric and fractional fail.
  for(const char* bad : {"99", "1000", "\"semibold\"", "true", "600.5", "[600]"}) {
    QByteArray weighted =
      withReplaced(validJson(), "\"senderWeight\": 600",
                   QByteArray("\"senderWeight\": ") + bad);
    Theme t = Theme::loadFromJson(weighted);
    QVERIFY2(!t.isValid, bad);
  }
  //  Legacy bool key is removed: a stale theme reads as invalid, not silent.
  QByteArray legacy = withReplaced(validJson(), "\"senderWeight\": 600", "\"senderBold\": true");
  QVERIFY(!Theme::loadFromJson(legacy).isValid);
}

void TstTheme::badHeaderColorRejected() {
  //  $refs died with the token map; red is not a hex; single/short/long
  //  arrays and gradients are not DualColors.
  for(const char* bad :
      {"\"$text\"", "\"red\"", "42", "[\"#1A1D21\"]", "[\"#1A1D21\", \"#E8EAED\", \"#000000\"]",
       "[\"#1A1D21\", \"red\"]", "\"gradient(#1A1D21, #E8EAED)\""}) {
    QByteArray colored =
      withReplaced(validJson(), "\"senderColor\": [\"#6B7280\", \"#9AA0A6\"]",
                   QByteArray("\"senderColor\": ") + bad);
    Theme t = Theme::loadFromJson(colored);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::headerColorsRequired() {
  //  No sane hardcoded default exists: the theme spells both.
  QByteArray noSender = withReplaced(validJson(), ",\n               \"senderColor\": [\"#6B7280\", \"#9AA0A6\"]", "");
  QVERIFY(!Theme::loadFromJson(noSender).isValid);
  QByteArray noTime = withReplaced(validJson(), ",\n               \"timeColor\": [\"#6B7280\", \"#9AA0A6\"]", "");
  QVERIFY(!Theme::loadFromJson(noTime).isValid);
}

void TstTheme::globalColorsParsed() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.text.light, QColor(QStringLiteral("#1A1D21")));
  QCOMPARE(t.text.dark, QColor(QStringLiteral("#E8EAED")));
  QCOMPARE(t.link.light, QColor(QStringLiteral("#0B5FFF")));
  QCOMPARE(t.link.dark, QColor(QStringLiteral("#8AB4F8")));
  QCOMPARE(t.page.light, QColor(QStringLiteral("#EDEFF2")));
  QCOMPARE(t.page.dark, QColor(QStringLiteral("#131518")));
}

void TstTheme::missingGlobalColorRejected() {
  for(const char* key : {"\"text\"", "\"link\"", "\"page\""}) {
    QByteArray base = validJson();
    //  Cut the whole line: "key": [...],
    int at = base.indexOf(QByteArray("\n    ") + key + ":");
    QVERIFY(at >= 0);
    int end = base.indexOf("\n", at + 1);
    base.remove(at, end - at);
    Theme t = Theme::loadFromJson(base);
    QVERIFY2(!t.isValid, key);
  }
}

void TstTheme::badGlobalColorRejected() {
  for(const char* bad : {"42", "\"red\"", "\"$text\"", "[\"#1A1D21\"]"}) {
    QByteArray colored = withReplaced(validJson(), "[\"#1A1D21\", \"#E8EAED\"]", bad);
    Theme t = Theme::loadFromJson(colored);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::rectMarginTopParsed() {
  //  MarginTop is a per-tree scalar: series-start trees carry the wide gap,
  //  _next trees the dense in-series one.
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(incomingBody(t).marginTop, 6);
  QCOMPARE(incomingNextBody(t).marginTop, 2);
}

void TstTheme::badMarginTopRejected() {
  //  MarginTop takes ints >= 0: negative, fractional, non-number fail.
  for(const char* bad : {"-1", "1.5", "\"big\"", "true"}) {
    QByteArray sized = withReplaced(validJson(), "\"marginTop\": 6",
                                    QByteArray("\"marginTop\": ") + bad);
    Theme t = Theme::loadFromJson(sized);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::splitEraShapesRejected() {
  //  Hard split: the {default, next?} marginTop/corners objects no longer
  //  parse, and neither do the flat legacy keys, the pre-rename "margin"
  //  key, or showOnGrouped.
  QByteArray marginTopObj =
    withReplaced(validJson(), "\"marginTop\": 6",
                 "\"marginTop\": {\"default\": 6, \"next\": 2}");
  QVERIFY(!Theme::loadFromJson(marginTopObj).isValid);
  QByteArray legacy = withReplaced(validJson(), "\"marginTop\": 6",
                                   "\"spacing\": 6, \"groupedSpacing\": 2");
  QVERIFY(!Theme::loadFromJson(legacy).isValid);
  QByteArray flat = withReplaced(
    validJson(), "\"marginTop\": 6",
    "\"marginTop\": 6, \"marginFirstInGroup\": 6, \"marginNextInGroup\": 2");
  QVERIFY(!Theme::loadFromJson(flat).isValid);
  //  Pre-rename "margin" reads as a stale key, not a silent miss.
  QByteArray renamed = withReplaced(validJson(), "\"marginTop\": 6", "\"margin\": 6");
  QVERIFY(!Theme::loadFromJson(renamed).isValid);
  QByteArray cornersObj = withReplaced(
    validJson(), "\"corners\": [12, 12, 12, 4]",
    "\"corners\": {\"default\": [12, 12, 12, 4], \"next\": [4, 12, 12, 4]}");
  QVERIFY(!Theme::loadFromJson(cornersObj).isValid);
  QByteArray showGrouped = withReplaced(validJson(), "\"shape\": \"circle\"",
                                        "\"shape\": \"circle\", \"showOnGrouped\": false");
  QVERIFY(!Theme::loadFromJson(showGrouped).isValid);
}

void TstTheme::cornerRadiiParsed() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& in = incomingBody(t);
  QVERIFY(in.hasCorners);
  const int inDef[4] = {12, 12, 12, 4};
  for(int i = 0; i < 4; i++)
    QCOMPARE(in.corners[i], inDef[i]);
  //  The in-series tree carries its own shape: window side flattened.
  const Theme::LayoutNode& inNext = incomingNextBody(t);
  QVERIFY(inNext.hasCorners);
  const int inNextWant[4] = {4, 12, 12, 4};
  for(int i = 0; i < 4; i++)
    QCOMPARE(inNext.corners[i], inNextWant[i]);
  //  Outgoing carries hand-mirrored values, read verbatim (no code mirror).
  //  The outgoing vbox holds header and body directly (no avatar child).
  const Theme::LayoutNode& outgoing = t.compiledLayout(QStringLiteral("layout_outgoing"));
  const Theme::LayoutNode& out = outgoing.children.at(1);
  const int outDef[4] = {12, 12, 4, 12};
  for(int i = 0; i < 4; i++)
    QCOMPARE(out.corners[i], outDef[i]);
  const Theme::LayoutNode& outgoingNext =
    t.compiledLayout(QStringLiteral("layout_outgoing_next"));
  const Theme::LayoutNode& outNext = outgoingNext.children.at(0);
  const int outNextWant[4] = {12, 4, 4, 12};
  for(int i = 0; i < 4; i++)
    QCOMPARE(outNext.corners[i], outNextWant[i]);
}

void TstTheme::cornersAbsentDefaulted() {
  //  A body node without corners stays valid and renders square;
  //  system/broadcast/request ship no corners.
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& system = t.compiledLayout(QStringLiteral("layout_system"));
  const Theme::LayoutNode& rect = system.children.at(0);
  QVERIFY(!rect.hasCorners);
  const int def[4] = {0, 0, 0, 0};
  for(int i = 0; i < 4; i++)
    QCOMPARE(rect.corners[i], def[i]);
}

void TstTheme::badCornerRadiiRejected() {
  //  Short, negative, fractional, non-array corners invalidate the theme.
  const char* cases[] = {
    "\"corners\": [12, 12]",
    "\"corners\": [-1, 12, 12, 4]",
    "\"corners\": [12, 12, 12, 4.5]",
    "\"corners\": \"round\"",
    "\"corners\": {\"default\": [12, 12, 12, 4]}",
  };
  for(const char* bad : cases) {
    QByteArray sized =
      withReplaced(validJson(), "\"corners\": [12, 12, 12, 4]", bad);
    Theme t = Theme::loadFromJson(sized);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::cornersOnNonRectRejected() {
  //  Corners paint the rect only: a header carrying them invalidates.
  QByteArray bad = withReplaced(validJson(), "{\"type\": \"avatar\"}",
                                "{\"type\": \"avatar\", "
                                "\"corners\": [1, 1, 1, 1]}");
  QVERIFY(!Theme::loadFromJson(bad).isValid);
}

void TstTheme::viewChromeDefaulted() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY(t.isValid);
  QCOMPARE(t.view.padding[0], 0);
  QCOMPARE(t.view.padding[1], 8);
  QCOMPARE(t.view.padding[2], 8);
  QCOMPARE(t.view.padding[3], 8);
}

void TstTheme::viewChromeParsed() {
  QByteArray base = withReplaced(validJson(), "\"version\": 1,",
                                 "\"version\": 1, \"viewPadding\": [4, 6, 10, 12],");
  Theme t = Theme::loadFromJson(base);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.view.padding[0], 4);
  QCOMPARE(t.view.padding[1], 6);
  QCOMPARE(t.view.padding[2], 10);
  QCOMPARE(t.view.padding[3], 12);
}

void TstTheme::badViewChromeRejected() {
  for(const char* bad : {"[-1, 8, 8, 8]", "[8, 8]", "\"big\"", "true"}) {
    QByteArray sized =
      withReplaced(validJson(), "\"version\": 1,",
                   QByteArray("\"version\": 1, \"viewPadding\": ") + bad + ",");
    Theme t = Theme::loadFromJson(sized);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::containerGapRequired() {
  //  Gap is the only spacing source: containers without it are invalid.
  Theme t = Theme::loadFromJson(withReplaced(validJson(), "\"type\": \"hbox\", \"gap\": 8",
                                             "\"type\": \"hbox\""));
  QVERIFY(!t.isValid);
  Theme t2 = Theme::loadFromJson(withReplaced(validJson(), "\"type\": \"vbox\", \"gap\": 2",
                                              "\"type\": \"vbox\""));
  QVERIFY(!t2.isValid);
}

void TstTheme::containerGapParsed() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& incoming = t.compiledLayout(QStringLiteral("layout_incoming"));
  QCOMPARE(incoming.type, Theme::LayoutNode::Type::HBox);
  QCOMPARE(incoming.gap, 8);
  QCOMPARE(incoming.children.size(), 2);
  QCOMPARE(incoming.children.at(0).type, Theme::LayoutNode::Type::Avatar);
  QCOMPARE(incoming.children.at(1).type, Theme::LayoutNode::Type::VBox);
  QCOMPARE(incoming.children.at(1).gap, 2);
  QCOMPARE(incoming.children.at(1).children.size(), 2);
  QCOMPARE(incoming.children.at(1).children.at(0).type,
           Theme::LayoutNode::Type::Header);
  QCOMPARE(incoming.children.at(1).children.at(1).type,
           Theme::LayoutNode::Type::Body);
  const Theme::LayoutNode& inBody = incoming.children.at(1).children.at(1);
  QVERIFY(inBody.hasCorners);
  QCOMPARE(inBody.corners[3], 4);
  QCOMPARE(inBody.children.size(), 0);
  //  The in-series tree omits header/avatar and carries its own shape:
  //  a vbox root holding the body directly.
  const Theme::LayoutNode& incomingNext =
    t.compiledLayout(QStringLiteral("layout_incoming_next"));
  QCOMPARE(incomingNext.type, Theme::LayoutNode::Type::VBox);
  QCOMPARE(incomingNext.gap, 0);
  QCOMPARE(incomingNext.children.size(), 1);
  QCOMPARE(incomingNext.children.at(0).type,
           Theme::LayoutNode::Type::Body);
  QCOMPARE(incomingNext.children.at(0).marginTop, 2);
  const Theme::LayoutNode& system = t.compiledLayout(QStringLiteral("layout_system"));
  QCOMPARE(system.type, Theme::LayoutNode::Type::VBox);
  QCOMPARE(system.gap, 0);
}

void TstTheme::badContainerGapRejected() {
  for(const char* bad : {"-1", "\"big\"", "true", "1.5"}) {
    QByteArray sized = withReplaced(validJson(), "\"type\": \"hbox\", \"gap\": 8",
                                    QByteArray("\"type\": \"hbox\", \"gap\": ") + bad);
    Theme t = Theme::loadFromJson(sized);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::staleThemeGapKeysIgnored() {
  //  Old files with theme-level gap keys stay valid; gaps come from nodes now.
  QByteArray base = withReplaced(validJson(), "\"size\": 40",
                                 "\"size\": 40, \"gap\": 10");
  base = withReplaced(base, "\"senderWeight\": 600", "\"senderWeight\": 600, \"gap\": 4");
  Theme t = Theme::loadFromJson(base);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.compiledLayout(QStringLiteral("layout_incoming")).gap, 8);
}

void TstTheme::fileCardOutsideRequestRejected() {
  //  fileCard lives in layout_request only: an incoming fileCard invalidates.
  QByteArray bad = withReplaced(
    validJson(), "{\"type\": \"body\", \"corners\": [12, 12, 12, 4]",
    "{\"type\": \"fileCard\", \"corners\": [12, 12, 12, 4]");
  Theme t = Theme::loadFromJson(bad);
  QVERIFY(!t.isValid);
}

void TstTheme::bodyInRequestRejected() {
  QByteArray bad = withReplaced(validJson(), "{\"type\": \"fileCard\", \"bg\"",
                                "{\"type\": \"body\", \"bg\"");
  Theme t = Theme::loadFromJson(bad);
  QVERIFY(!t.isValid);
}

void TstTheme::twoRectsInLayoutRejected() {
  //  One rect per layout: a second body/fileCard invalidates the theme,
  //  even in the file layout.
  QByteArray bad = withReplaced(
    validJson(), "\"marginTop\": 6, \"maxWidthPct\": 75}",
    "\"marginTop\": 6, \"maxWidthPct\": 75}, {\"type\": \"body\", \"bg\": \"#FFFFFF\", \"padding\": [8, 10, 8, 10], \"shadow\": [0, 2, 0.18], \"marginTop\": 6, \"maxWidthPct\": 75}");
  Theme t = Theme::loadFromJson(bad);
  QVERIFY(!t.isValid);
  QByteArray bad2 = withReplaced(
    validJson(), "{\"type\": \"fileCard\", \"bg\"",
    "{\"type\": \"body\", \"bg\"}, {\"type\": \"fileCard\", \"bg\"");
  Theme t2 = Theme::loadFromJson(bad2);
  QVERIFY(!t2.isValid);
}

void TstTheme::requestFileCardAccepted() {
  //  The file layout holds an unconditional fileCard rect, no body.
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& request = t.compiledLayout(QStringLiteral("layout_request"));
  QCOMPARE(request.children.size(), 1);
  const Theme::LayoutNode& rect = request.children.at(0);
  QCOMPARE(rect.type, Theme::LayoutNode::Type::FileCard);
  QCOMPARE(rect.children.size(), 0);
  QVERIFY(rect.hasBorder);
  QCOMPARE(rect.padding[0], 8);
  QCOMPARE(rect.padding[1], 10);
  QCOMPARE(rect.shadowOpacity, 0.18);
  QCOMPARE(rect.marginTop, 6);
  QCOMPARE(rect.maxWidthPct, 75);
}

void TstTheme::bareHexBothModes() {
  //  Form 1: one hex paints both modes (outgoing text ships bare).
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& outgoing = t.compiledLayout(QStringLiteral("layout_outgoing"));
  const Theme::LayoutNode& rect = outgoing.children.at(1);
  QVERIFY(rect.hasTextColor);
  QCOMPARE(rect.textColor.light, QColor(QStringLiteral("#1A1D21")));
  QCOMPARE(rect.textColor.dark, QColor(QStringLiteral("#1A1D21")));
}

void TstTheme::pairParsed() {
  //  Form 2: [light, dark] splits the modes (incoming sender ships paired).
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& incoming = t.compiledLayout(QStringLiteral("layout_incoming"));
  const Theme::LayoutNode& header = incoming.children.at(1).children.at(0);
  QVERIFY(header.hasSenderColor);
  QCOMPARE(header.senderColor.light, QColor(QStringLiteral("#1A1D21")));
  QCOMPARE(header.senderColor.dark, QColor(QStringLiteral("#E8EAED")));
  QVERIFY(!header.hasTimeColor);
}

void TstTheme::gradientParsed() {
  //  Form 3: per-mode gradients with exact stops (incoming bg).
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& rect = incomingBody(t);
  QCOMPARE(rect.bg.kind, Theme::Bg::Kind::Gradient);
  QCOMPARE(rect.bg.lightFrom, QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(rect.bg.lightTo, QColor(QStringLiteral("#F1F4F8")));
  QCOMPARE(rect.bg.darkFrom, QColor(QStringLiteral("#1F242B")));
  QCOMPARE(rect.bg.darkTo, QColor(QStringLiteral("#262D36")));
  QCOMPARE(rect.maxWidthPct, 75);
  QVERIFY(!rect.hasTextColor);
  QVERIFY(!rect.hasBorder);
}

void TstTheme::solidKindsParsed() {
  //  Solid pair (broadcast) and bare solid (outgoing is a gradient;
  //  broadcast bg is a pair of plain hexes).
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& broadcast = t.compiledLayout(QStringLiteral("layout_broadcast"));
  const Theme::LayoutNode& rect = broadcast.children.at(0);
  QCOMPARE(rect.bg.kind, Theme::Bg::Kind::Solid);
  QCOMPARE(rect.bg.solid.light, QColor(QStringLiteral("#E6F4EA")));
  QCOMPARE(rect.bg.solid.dark, QColor(QStringLiteral("#1A2E22")));
  QVERIFY(rect.hasBorder);
  QCOMPARE(rect.border.light, QColor(QStringLiteral("#2F9E44")));
  QCOMPARE(rect.border.dark, QColor(QStringLiteral("#3FB950")));
}

void TstTheme::transparentAccepted() {
  //  "transparent" survives only as a body/fileCard bg (system look).
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& system = t.compiledLayout(QStringLiteral("layout_system"));
  const Theme::LayoutNode& rect = system.children.at(0);
  QCOMPARE(rect.bg.kind, Theme::Bg::Kind::Transparent);
  QColor first, second;
  Theme::pickBg(rect.bg, false, first, second);
  QCOMPARE(first, QColor(Qt::transparent));
  QCOMPARE(second, QColor(Qt::transparent));
  Theme::pickBg(rect.bg, true, first, second);
  QCOMPARE(first, QColor(Qt::transparent));
  //  System text ships paired.
  QVERIFY(rect.hasTextColor);
  QCOMPARE(rect.textColor.dark, QColor(QStringLiteral("#9AA0A6")));
}

void TstTheme::transparentOutsideBgRejected() {
  QByteArray inText = withReplaced(validJson(), "\"textColor\": [\"#6B7280\", \"#9AA0A6\"]",
                                   "\"textColor\": \"transparent\"");
  QVERIFY(!Theme::loadFromJson(inText).isValid);
  QByteArray inBorder = withReplaced(validJson(), "\"border\": [\"#2F9E44\", \"#3FB950\"]",
                                     "\"border\": \"transparent\"");
  QVERIFY(!Theme::loadFromJson(inBorder).isValid);
  QByteArray inPair = withReplaced(validJson(), "[\"#1A1D21\", \"#E8EAED\"]",
                                   "[\"transparent\", \"#E8EAED\"]");
  QVERIFY(!Theme::loadFromJson(inPair).isValid);
}

void TstTheme::mixedPairRejected() {
  //  A bg pair mixes either two solids or two gradients, never one of each.
  QByteArray mixed = withReplaced(
    validJson(), "\"bg\": [\"gradient(#FFFFFF, #F1F4F8)\", \"gradient(#1F242B, #262D36)\"]",
    "\"bg\": [\"#FFFFFF\", \"gradient(#1F242B, #262D36)\"]");
  QVERIFY(!Theme::loadFromJson(mixed).isValid);
  QByteArray mixed2 = withReplaced(
    validJson(), "\"bg\": [\"#E6F4EA\", \"#1A2E22\"]",
    "\"bg\": [\"#E6F4EA\", \"gradient(#1A2E22, #1A2E22)\"]");
  QVERIFY(!Theme::loadFromJson(mixed2).isValid);
}

void TstTheme::badGradientRejected() {
  //  Arity is exactly two bare hex stops inside gradient(...).
  for(const char* bad :
      {"\"gradient(#FFFFFF)\"", "\"gradient(#FFFFFF, #F1F4F8, #000000)\"",
       "\"gradient(red, #F1F4F8)\"", "\"gradient(#FFFFFF #F1F4F8)\"",
       "\"gradient#FFFFFF, #F1F4F8)\"", "\"gradient(#FFFFFF, #F1F4F8\"",
       "\"linear-gradient(#FFFFFF, #F1F4F8)\"", "\"gradient()\"",
       "\"gradient(#FFF, #F1F4F8)\""}) {
    QByteArray base = withReplaced(
      validJson(), "\"bg\": [\"gradient(#FFFFFF, #F1F4F8)\", \"gradient(#1F242B, #262D36)\"]",
      QByteArray("\"bg\": [") + bad + ", \"gradient(#1F242B, #262D36)\"]");
    Theme t = Theme::loadFromJson(base);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::gradientWhitespaceAccepted() {
  //  Spaces around the tokens are tolerated, not significant.
  QByteArray spaced = withReplaced(
    validJson(), "\"bg\": [\"gradient(#FFFFFF, #F1F4F8)\", \"gradient(#1F242B, #262D36)\"]",
    "\"bg\": [\"gradient( #FFFFFF , #F1F4F8 )\", \"gradient(#1F242B,#262D36)\"]");
  Theme t = Theme::loadFromJson(spaced);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& rect = incomingBody(t);
  QCOMPARE(rect.bg.lightFrom, QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(rect.bg.darkTo, QColor(QStringLiteral("#262D36")));
}

void TstTheme::gradientOutsideBgRejected() {
  //  Gradients paint fills only: text, border and header colors take solids.
  QByteArray inText = withReplaced(validJson(), "\"textColor\": \"#1A1D21\"",
                                   "\"textColor\": \"gradient(#1A1D21, #E8EAED)\"");
  QVERIFY(!Theme::loadFromJson(inText).isValid);
  QByteArray inBorder = withReplaced(
    validJson(), "\"border\": [\"#2F9E44\", \"#3FB950\"]",
    "\"border\": [\"gradient(#2F9E44, #1A2E22)\", \"gradient(#3FB950, #1A2E22)\"]");
  QVERIFY(!Theme::loadFromJson(inBorder).isValid);
  QByteArray inHeader = withReplaced(
    validJson(), "\"senderColor\": [\"#1A1D21\", \"#E8EAED\"]",
    "\"senderColor\": [\"gradient(#1A1D21, #000000)\", \"gradient(#E8EAED, #FFFFFF)\"]");
  QVERIFY(!Theme::loadFromJson(inHeader).isValid);
}

void TstTheme::missingBgDefaulted() {
  //  Absent bg = transparent (flat text on the page).
  QByteArray noBg = withReplaced(
    validJson(),
    "\"bg\": [\"gradient(#FFFFFF, #F1F4F8)\", \"gradient(#1F242B, #262D36)\"], \"padding\"",
    "\"padding\"");
  Theme t = Theme::loadFromJson(noBg);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& rect = incomingBody(t);
  QVERIFY(!rect.hasBg);
  QCOMPARE(rect.bg.kind, Theme::Bg::Kind::Transparent);
  //  Present-but-bad bg still invalidates the theme.
  QVERIFY(!Theme::loadFromJson(withReplaced(validJson(), "\"bg\": [\"#E6F4EA\", \"#1A2E22\"]",
                                            "\"bg\": \"$bubbleIn\""))
             .isValid);
}

void TstTheme::missingMaxWidthPctDefaulted() {
  //  Absent maxWidthPct = full width.
  QByteArray noWidth = validJson();
  noWidth.replace(", \"maxWidthPct\": 75", "");
  noWidth.replace(", \"maxWidthPct\": 84", "");
  noWidth.replace(", \"maxWidthPct\": 88", "");
  noWidth.replace(", \"maxWidthPct\": 80", "");
  Theme t = Theme::loadFromJson(noWidth);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(incomingBody(t).maxWidthPct, 100);
  //  Present-but-bad width still invalidates the theme.
  QVERIFY(!Theme::loadFromJson(withReplaced(validJson(), "\"maxWidthPct\": 75",
                                            "\"maxWidthPct\": 0"))
             .isValid);
  QVERIFY(!Theme::loadFromJson(withReplaced(validJson(), "\"maxWidthPct\": 75",
                                            "\"maxWidthPct\": 101"))
             .isValid);
  QVERIFY(!Theme::loadFromJson(withReplaced(validJson(), "\"maxWidthPct\": 75",
                                            "\"maxWidthPct\": 75.5"))
             .isValid);
  QVERIFY(!Theme::loadFromJson(withReplaced(validJson(), "\"maxWidthPct\": 75",
                                            "\"maxWidthPct\": 84.9"))
             .isValid);
}

void TstTheme::rectMetricsParsed() {
  //  Metrics ride the rect node: the incoming rect ships fixture values.
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& rect = incomingBody(t);
  QCOMPARE(rect.padding[0], 8);
  QCOMPARE(rect.padding[1], 10);
  QCOMPARE(rect.padding[2], 8);
  QCOMPARE(rect.padding[3], 10);
  QCOMPARE(rect.shadowDx, 0.0);
  QCOMPARE(rect.shadowDy, 2.0);
  QCOMPARE(rect.shadowOpacity, 0.18);
  QCOMPARE(rect.marginTop, 6);
  QCOMPARE(rect.maxWidthPct, 75);
}

void TstTheme::rectMetricsOptionalDefaulted() {
  //  padding/shadow/marginTop are optional: absent = zeros (flat, no gap).
  QByteArray noMetrics = validJson();
  noMetrics.replace(", \"padding\": [8, 10, 8, 10]", "");
  noMetrics.replace(", \"shadow\": [0, 2, 0.18]", "");
  noMetrics.replace(", \"marginTop\": 6", "");
  noMetrics.replace(", \"marginTop\": 2", "");
  Theme t = Theme::loadFromJson(noMetrics);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& rect = incomingBody(t);
  QCOMPARE(rect.padding[0], 0);
  QCOMPARE(rect.padding[1], 0);
  QCOMPARE(rect.padding[2], 0);
  QCOMPARE(rect.padding[3], 0);
  QCOMPARE(rect.shadowDx, 0.0);
  QCOMPARE(rect.shadowDy, 0.0);
  QCOMPARE(rect.shadowOpacity, 0.0);
  QCOMPARE(rect.marginTop, 0);
  QCOMPARE(rect.maxWidthPct, 75);
  QCOMPARE(incomingNextBody(t).marginTop, 0);
  //  Present-but-bad values still invalidate the theme.
  QVERIFY(!Theme::loadFromJson(withReplaced(validJson(), "\"padding\": [8, 10, 8, 10]",
                                            "\"padding\": [8, 10, 8]"))
             .isValid);
  QVERIFY(!Theme::loadFromJson(withReplaced(validJson(), "\"shadow\": [0, 2, 0.18]",
                                            "\"shadow\": [0, 2, 1.5]"))
             .isValid);
  //  Legacy 4-element [dx, dy, blur, opacity] is rejected: no blur.
  QVERIFY(!Theme::loadFromJson(withReplaced(validJson(), "\"shadow\": [0, 2, 0.18]",
                                            "\"shadow\": [0, 2, 0, 0.18]"))
             .isValid);
  QVERIFY(!Theme::loadFromJson(withReplaced(validJson(), "\"marginTop\": 6", "\"marginTop\": -1"))
             .isValid);
}

void TstTheme::tailRejected() {
  //  Rows are tailless: any tail key on any node invalidates the theme.
  for(const char* key : {"\"tail\": \"left\"", "\"tailSize\": 8", "\"tailStyle\": \"polygon\""}) {
    QByteArray tailed = withReplaced(validJson(), "\"maxWidthPct\": 75}",
                                     QByteArray("\"maxWidthPct\": 75, ") + key + "}");
    Theme t = Theme::loadFromJson(tailed);
    QVERIFY2(!t.isValid, key);
  }
}

void TstTheme::colorKeyOnWrongNodeRejected() {
  //  Keys name the painted element: bg/text/border/metrics live on
  //  body/fileCard, sender/time on headers, tails nowhere.
  QByteArray bgOnHeader = withReplaced(
    validJson(), "{\"type\": \"header\", \"items\": [\"sender\", \"time\"]",
    "{\"type\": \"header\", \"items\": [\"sender\", \"time\"], \"bg\": \"#FFFFFF\"");
  QVERIFY(!Theme::loadFromJson(bgOnHeader).isValid);
  QByteArray senderOnRect = withReplaced(
    validJson(), "\"textColor\": \"#1A1D21\"",
    "\"textColor\": \"#1A1D21\", \"senderColor\": \"#FFFFFF\"");
  QVERIFY(!Theme::loadFromJson(senderOnRect).isValid);
  QByteArray textOnAvatar = withReplaced(validJson(), "{\"type\": \"avatar\"}",
                                         "{\"type\": \"avatar\", \"textColor\": \"#FFFFFF\"}");
  QVERIFY(!Theme::loadFromJson(textOnAvatar).isValid);
  QByteArray tailOnHeader = withReplaced(
    validJson(), "{\"type\": \"header\", \"items\": [\"sender\", \"time\"]",
    "{\"type\": \"header\", \"items\": [\"sender\", \"time\"], \"tail\": \"left\"");
  QVERIFY(!Theme::loadFromJson(tailOnHeader).isValid);
}

void TstTheme::dollarRefsRejected() {
  //  No $ indirection remains anywhere: refs are literals now.
  for(const char* bad : {"\"$text\"", "\"$bubbleIn\""}) {
    QByteArray refed = withReplaced(validJson(), "\"textColor\": \"#1A1D21\"",
                                    QByteArray("\"textColor\": ") + bad);
    QVERIFY2(!Theme::loadFromJson(refed).isValid, bad);
  }
  QByteArray refedBg = withReplaced(
    validJson(), "\"bg\": [\"#E6F4EA\", \"#1A2E22\"]", "\"bg\": \"$bubbleIn\"");
  QVERIFY(!Theme::loadFromJson(refedBg).isValid);
}

void TstTheme::layoutWithoutRectRejected() {
  //  The delegate always paints one rect: a layout without body/fileCard
  //  is invalid.
  QByteArray noRect = withReplaced(
    validJson(), "{\"type\": \"body\", \"bg\": \"transparent\", \"textColor\": [\"#6B7280\", \"#9AA0A6\"], \"padding\": [8, 10, 8, 10], \"shadow\": [0, 2, 0.18], \"marginTop\": 6, \"maxWidthPct\": 75}",
    "{\"type\": \"header\", \"items\": [\"time\"]}");
  Theme t = Theme::loadFromJson(noRect);
  QVERIFY(!t.isValid);
}

void TstTheme::missingLayoutRejected() {
  //  Renaming a layout key reads as a missing layout: all 7 are required.
  Theme t = Theme::loadFromJson(withReplaced(validJson(), "\"layout_request\":",
                                             "\"layout_nope\":"));
  QVERIFY(!t.isValid);
  //  The in-series trees are required too: no silent single-tree fallback.
  Theme t2 = Theme::loadFromJson(withReplaced(validJson(), "\"layout_incoming_next\":",
                                              "\"layout_nope\":"));
  QVERIFY(!t2.isValid);
}

void TstTheme::builtInDefaultValid() {
  //  defaultJsonText pins the resource route (no silent fallback).
  QVERIFY2(!Theme::defaultJsonText().isEmpty(), "built-in ClassicSlate resource unreadable");
  Theme t = Theme::builtInDefault();
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.compiled.size(), 7);
  QVERIFY(t.text.light.isValid());
  QVERIFY(t.page.dark.isValid());
}

void TstTheme::pickHelpers() {
  Theme::DualColor c{QColor(QStringLiteral("#111111")), QColor(QStringLiteral("#EEEEEE"))};
  QCOMPARE(Theme::pick(c, false), QColor(QStringLiteral("#111111")));
  QCOMPARE(Theme::pick(c, true), QColor(QStringLiteral("#EEEEEE")));
  QCOMPARE(Theme::isDark(Theme::Mode::Light, true), false);
  QCOMPARE(Theme::isDark(Theme::Mode::Dark, false), true);
  QCOMPARE(Theme::isDark(Theme::Mode::System, false), false);
  QCOMPARE(Theme::isDark(Theme::Mode::System, true), true);

  Theme t = Theme::loadFromJson(validJson());
  QVERIFY(t.isValid);
  const Theme::LayoutNode& rect = incomingBody(t);
  QColor first, second;
  Theme::pickBg(rect.bg, false, first, second);
  QCOMPARE(first, QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(second, QColor(QStringLiteral("#F1F4F8")));
  Theme::pickBg(rect.bg, true, first, second);
  QCOMPARE(first, QColor(QStringLiteral("#1F242B")));
  QCOMPARE(second, QColor(QStringLiteral("#262D36")));
  //  Solid: both stops coincide.
  const Theme::LayoutNode& broadcast = t.compiledLayout(QStringLiteral("layout_broadcast"));
  Theme::pickBg(broadcast.children.at(0).bg, true, first, second);
  QCOMPARE(first, second);
  QCOMPARE(first, QColor(QStringLiteral("#1A2E22")));
}

void TstTheme::shippedMeshBlueSpotCheck() {
  //  The migration expanded tokens 1:1: incoming gradient and outgoing
  //  bare-white text read back exactly.
  QDir themesDir(QString::fromLocal8Bit(LMC_THEME_DIR));
  QFile f(themesDir.absoluteFilePath(QStringLiteral("MeshBlue.json")));
  QVERIFY(f.open(QIODevice::ReadOnly));
  Theme t = Theme::loadFromJson(f.readAll());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  const Theme::LayoutNode& incoming = t.compiledLayout(QStringLiteral("layout_incoming"));
  const Theme::LayoutNode& in = incoming.children.at(1).children.at(1);
  QCOMPARE(in.bg.kind, Theme::Bg::Kind::Gradient);
  QCOMPARE(in.bg.lightFrom, QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(in.bg.lightTo, QColor(QStringLiteral("#F0FDF4")));
  QCOMPARE(in.bg.darkFrom, QColor(QStringLiteral("#162235")));
  QCOMPARE(in.bg.darkTo, QColor(QStringLiteral("#111A29")));
  //  Shipped incoming carries the spec 1px border (Stitch chat spec).
  QVERIFY(in.hasBorder);
  QCOMPARE(in.border.light, QColor(QStringLiteral("#E2E8F0")));
  QCOMPARE(in.border.dark, QColor(QStringLiteral("#1E3A5F")));
  const Theme::LayoutNode& outgoing = t.compiledLayout(QStringLiteral("layout_outgoing"));
  const Theme::LayoutNode& out = outgoing.children.at(1);
  QVERIFY(out.hasTextColor);
  QCOMPARE(out.textColor.light, QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(out.textColor.dark, QColor(QStringLiteral("#FFFFFF")));
}

void TstTheme::availableThemesListsUserAndSkipsBroken() {
  QStandardPaths::setTestModeEnabled(true);
  QString base = StdLocation::userThemeDir();
  QVERIFY(QDir().mkpath(base));
  QFile good(base + QStringLiteral("/Good.json"));
  QVERIFY(good.open(QIODevice::WriteOnly));
  good.write(validJson());
  good.close();
  QFile bad(base + QStringLiteral("/Bad.json"));
  QVERIFY(bad.open(QIODevice::WriteOnly));
  bad.write("{broken");
  bad.close();

  QStringList themes = Theme::availableThemes();
  QVERIFY(themes.contains(QStringLiteral("ClassicSlate")));
  QVERIFY(themes.contains(QStringLiteral("Good")));
  QVERIFY(!themes.contains(QStringLiteral("Bad")));
  QCOMPARE(Theme::themeFileFor(QStringLiteral("Good")),
           QDir(base).absoluteFilePath(QStringLiteral("Good.json")));
  QVERIFY(Theme::themeFileFor(QStringLiteral("Nope")).isEmpty());

  QVERIFY(QFile::remove(base + QStringLiteral("/Good.json")));
  QVERIFY(QFile::remove(base + QStringLiteral("/Bad.json")));
  QStandardPaths::setTestModeEnabled(false);
}

void TstTheme::realFileLoads() {
  QDir themesDir(QString::fromLocal8Bit(LMC_THEME_DIR));
  const QStringList files =
    themesDir.entryList(QStringList(QStringLiteral("*.json")), QDir::Files, QDir::Name);
  QVERIFY2(!files.isEmpty(), "no themes found");
  QVERIFY(files.contains(QStringLiteral("ClassicSlate.json")));
  for(const QString& file : files) {
    QFile f(themesDir.absoluteFilePath(file));
    QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(QStringLiteral("cannot open ") + file));
    Theme t = Theme::loadFromJson(f.readAll());
    QVERIFY2(t.isValid, qPrintable(file + QStringLiteral(": ") + t.errorString));
  }
}

QTEST_MAIN(TstTheme)
#include "tst_theme.moc"
