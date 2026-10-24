// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
// Unit tests for the Theme validator (Bubble, stage 1).

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include "core/stdlocation.h"
#include "core/theme.h"

namespace {

QByteArray validJson() {
  return QByteArray(R"({
    "version": 1,
    "bubble": {"radius": [12, 12, 12, 4], "tail": 8, "shadow": [0, 2, 8, 0.18],
               "padding": [8, 10, 8, 10], "margin": 6, "marginFirstInGroup": 6,
               "marginNextInGroup": 2, "maxWidthPct": 75},
    "avatar": {"size": 40, "shape": "circle", "showOnGrouped": false},
    "header": {"senderBold": true, "timeFormat": "HH:mm"},
    "light": {"bubbleIn": "#FFFFFF", "bubbleIn2": "#F1F4F8", "bubbleOut": "#D9F2C7",
              "bubbleOut2": "#C9E8B4", "text": "#1A1D21", "secondary": "#6B7280",
              "link": "#0B5FFF", "border": "#D8DEE5", "page": "#EDEFF2",
              "accent": "#2F9E44", "accentSoft": "#E6F4EA", "textInverse": "#1A1D21"},
    "dark": {"bubbleIn": "#1F242B", "bubbleIn2": "#262D36", "bubbleOut": "#1D4D2B",
             "bubbleOut2": "#1A4527", "text": "#E8EAED", "secondary": "#9AA0A6",
             "link": "#8AB4F8", "border": "#3A424C", "page": "#131518",
             "accent": "#3FB950", "accentSoft": "#1A2E22", "textInverse": "#E8EAED"},
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

class TstTheme : public QObject {
  Q_OBJECT

private slots:
  void validTheme();
  void badVersion();
  void missingTwelfthTokenRejected();
  void optionalKeysDefaulted();
  void outgoingYouLabelParsed();
  void outgoingYouLabelExplicitFalse();
  void headerTypographyDefaulted();
  void headerTypographyParsed();
  void badHeaderSizeRejected();
  void badHeaderColorRejected();
  void bubbleMarginsDefaulted();
  void bubbleMarginsParsed();
  void badMarginRejected();
  void legacySpacingRejected();
  void viewChromeDefaulted();
  void viewChromeParsed();
  void badViewChromeRejected();
  void badTailStyleRejected();
  void roundedTailStyleAccepted();
  void systemPillAccepted();
  void pillAsymmetricRejected();
  void cardTextAccepted();
  void hexInCardsRejected();
  void transparentOutsideSystemRejected();
  void unknownTokenRejected();
  void missingLayoutRejected();
  void reservedBlocksAcceptedButIgnored();
  void badFlagRejected();
  void tooDeepRejected();
  void fractionalArrayRejected();
  void availableThemesListsUserAndSkipsBroken();
  void resolveTokens();
  void realFileLoads();
};

void TstTheme::validTheme() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.bubble.maxWidthPct, 75);
  QCOMPARE(t.avatar.size, 40);
  QCOMPARE(t.header.timeFormat, QStringLiteral("HH:mm"));
  QVERIFY(t.hasToken(QStringLiteral("bubbleIn")));
  QCOMPARE(t.layouts.size(), 5);
}

void TstTheme::optionalKeysDefaulted() {
  //  Old 12-token files without the new keys stay valid: no outgoing label,
  //  polygon tail (Bubble behavior), no pill tokens.
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY(t.isValid);
  QVERIFY(!t.header.outgoingYouLabel);
  QCOMPARE(t.bubble.tailStyle, QStringLiteral("polygon"));
  QVERIFY(!t.hasToken(QStringLiteral("pill")));
  QVERIFY(!t.hasToken(QStringLiteral("pillBorder")));
}

void TstTheme::outgoingYouLabelParsed() {
  QByteArray labeled = withReplaced(validJson(), "\"timeFormat\": \"HH:mm\"}",
                                    "\"timeFormat\": \"HH:mm\", \"outgoingYouLabel\": true}");
  Theme t = Theme::loadFromJson(labeled);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QVERIFY(t.header.outgoingYouLabel);
}

void TstTheme::outgoingYouLabelExplicitFalse() {
  QByteArray base = withReplaced(validJson(), "\"timeFormat\": \"HH:mm\"}",
                                 "\"timeFormat\": \"HH:mm\", \"outgoingYouLabel\": false}");
  Theme t = Theme::loadFromJson(base);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QVERIFY(!t.header.outgoingYouLabel);
}

void TstTheme::headerTypographyDefaulted() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY(t.isValid);
  QCOMPARE(t.header.senderSize, 1.0);
  QCOMPARE(t.header.timeSize, 1.0);
  QCOMPARE(t.header.senderColor, QStringLiteral("$secondary"));
  QCOMPARE(t.header.timeColor, QStringLiteral("$secondary"));
}

void TstTheme::headerTypographyParsed() {
  QByteArray typed = withReplaced(validJson(), "\"senderBold\": true", "\"senderBold\": false");
  typed = withReplaced(typed, "\"timeFormat\": \"HH:mm\"}",
                       "\"timeFormat\": \"HH:mm\", \"senderSize\": 1.25, \"timeSize\": 0.75, "
                       "\"senderColor\": \"$text\", \"timeColor\": \"$link\"}");
  Theme t = Theme::loadFromJson(typed);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QVERIFY(!t.header.senderBold);
  QCOMPARE(t.header.senderSize, 1.25);
  QCOMPARE(t.header.timeSize, 0.75);
  QCOMPARE(t.header.senderColor, QStringLiteral("$text"));
  QCOMPARE(t.header.timeColor, QStringLiteral("$link"));
}

void TstTheme::badHeaderSizeRejected() {
  for(const char* bad : {"0.1", "2.5", "\"big\"", "true"}) {
    QByteArray sized =
      withReplaced(validJson(), "\"timeFormat\": \"HH:mm\"}",
                   QByteArray("\"timeFormat\": \"HH:mm\", \"senderSize\": ") + bad + "}");
    Theme t = Theme::loadFromJson(sized);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::badHeaderColorRejected() {
  for(const char* bad : {"\"#1A1D21\"", "\"$nope\"", "42"}) {
    QByteArray colored =
      withReplaced(validJson(), "\"timeFormat\": \"HH:mm\"}",
                   QByteArray("\"timeFormat\": \"HH:mm\", \"senderColor\": ") + bad + "}");
    Theme t = Theme::loadFromJson(colored);
    QVERIFY2(!t.isValid, bad);
  }
}

void TstTheme::bubbleMarginsDefaulted() {
  //  Positional margins fall back to margin when absent.
  QByteArray base = withReplaced(validJson(), ", \"marginFirstInGroup\": 6,\n"
                                              "               \"marginNextInGroup\": 2",
                                 "");
  Theme t = Theme::loadFromJson(base);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.bubble.margin, 6);
  QCOMPARE(t.bubble.marginFirstInGroup, 6);
  QCOMPARE(t.bubble.marginNextInGroup, 6);
}

void TstTheme::bubbleMarginsParsed() {
  QByteArray explicitMargins = withReplaced(
    validJson(), "\"margin\": 6, \"marginFirstInGroup\": 6,\n"
                 "               \"marginNextInGroup\": 2",
    "\"margin\": 4, \"marginFirstInGroup\": 8, \"marginNextInGroup\": 3");
  Theme t = Theme::loadFromJson(explicitMargins);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.bubble.margin, 4);
  QCOMPARE(t.bubble.marginFirstInGroup, 8);
  QCOMPARE(t.bubble.marginNextInGroup, 3);
}

void TstTheme::badMarginRejected() {
  struct KeyValue {
    const char* key;
    const char* value;
  };
  const KeyValue keys[] = {{"margin", "6"},
                           {"marginFirstInGroup", "6"},
                           {"marginNextInGroup", "2"}};
  for(KeyValue kv : keys) {
    for(const char* bad : {"-1", "\"big\"", "true"}) {
      QByteArray sized = withReplaced(validJson(), QByteArray("\"") + kv.key + "\": " + kv.value,
                                      QByteArray("\"") + kv.key + "\": " + bad);
      Theme t = Theme::loadFromJson(sized);
      QVERIFY2(!t.isValid, (QByteArray(kv.key) + "=" + bad).constData());
    }
  }
}

void TstTheme::legacySpacingRejected() {
  //  Hard rename: spacing/groupedSpacing no longer parse, margin is required.
  QByteArray legacy = withReplaced(
    validJson(), "\"margin\": 6, \"marginFirstInGroup\": 6,\n"
                 "               \"marginNextInGroup\": 2",
    "\"spacing\": 6, \"groupedSpacing\": 2");
  Theme t = Theme::loadFromJson(legacy);
  QVERIFY(!t.isValid);
}

void TstTheme::viewChromeDefaulted() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY(t.isValid);
  QCOMPARE(t.view.padding[0], 0);
  QCOMPARE(t.view.padding[1], 8);
  QCOMPARE(t.view.padding[2], 8);
  QCOMPARE(t.view.padding[3], 8);
  QCOMPARE(t.avatar.gap, 8);
  QCOMPARE(t.header.gap, 2);
}

void TstTheme::viewChromeParsed() {
  QByteArray base = withReplaced(validJson(), "\"version\": 1,",
                                 "\"version\": 1, \"viewPadding\": [4, 6, 10, 12],");
  base = withReplaced(base, "\"showOnGrouped\": false",
                      "\"showOnGrouped\": false, \"gap\": 10");
  base =
    withReplaced(base, "\"senderBold\": true", "\"senderBold\": true, \"gap\": 4");
  Theme t = Theme::loadFromJson(base);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.view.padding[0], 4);
  QCOMPARE(t.view.padding[1], 6);
  QCOMPARE(t.view.padding[2], 10);
  QCOMPARE(t.view.padding[3], 12);
  QCOMPARE(t.avatar.gap, 10);
  QCOMPARE(t.header.gap, 4);
}

void TstTheme::badViewChromeRejected() {
  for(const char* bad : {"[-1, 8, 8, 8]", "[8, 8]", "\"big\"", "true"}) {
    QByteArray sized =
      withReplaced(validJson(), "\"version\": 1,",
                   QByteArray("\"version\": 1, \"viewPadding\": ") + bad + ",");
    Theme t = Theme::loadFromJson(sized);
    QVERIFY2(!t.isValid, bad);
  }
  for(const char* bad : {"-1", "\"big\"", "true"}) {
    Theme ta =
      Theme::loadFromJson(withReplaced(validJson(), "\"showOnGrouped\": false",
                                       QByteArray("\"showOnGrouped\": false, \"gap\": ") + bad));
    QVERIFY2(!ta.isValid, bad);
    Theme th = Theme::loadFromJson(withReplaced(validJson(), "\"senderBold\": true",
                                                QByteArray("\"senderBold\": true, \"gap\": ") + bad));
    QVERIFY2(!th.isValid, bad);
  }
}

void TstTheme::badTailStyleRejected() {
  QByteArray bad =
    withReplaced(validJson(), "\"tail\": 8,", "\"tail\": 8, \"tailStyle\": \"spike\",");
  Theme t = Theme::loadFromJson(bad);
  QVERIFY(!t.isValid);
}

void TstTheme::roundedTailStyleAccepted() {
  QByteArray rounded =
    withReplaced(validJson(), "\"tail\": 8,", "\"tail\": 2, \"tailStyle\": \"rounded\",");
  Theme t = Theme::loadFromJson(rounded);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.bubble.tail, 2);
  QCOMPARE(t.bubble.tailStyle, QStringLiteral("rounded"));
}

void TstTheme::systemPillAccepted() {
  //  System pill: optional tokens in both palettes + $refs in the system card.
  QByteArray base = withReplaced(validJson(), ", \"textInverse\": \"#1A1D21\"",
                                 ", \"textInverse\": \"#1A1D21\", "
                                 "\"pill\": \"#E8EDF2\", \"pillBorder\": \"#E2E8F0\"");
  base = withReplaced(base, ", \"textInverse\": \"#E8EAED\"",
                      ", \"textInverse\": \"#E8EAED\", "
                      "\"pill\": \"#1E293B\", \"pillBorder\": \"#334155\"");
  base = withReplaced(base, "\"system\": {\"bg\": \"transparent\"",
                      "\"system\": {\"bg\": \"$pill\", \"border\": \"$pillBorder\"");
  Theme t = Theme::loadFromJson(base);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.resolve(QStringLiteral("$pill"), Theme::Mode::Light, false),
           QColor(QStringLiteral("#E8EDF2")));
  QCOMPARE(t.resolve(QStringLiteral("$pillBorder"), Theme::Mode::Dark, false),
           QColor(QStringLiteral("#334155")));
  QCOMPARE(t.cards.value(QStringLiteral("system")).bg, QStringLiteral("$pill"));
}

void TstTheme::pillAsymmetricRejected() {
  //  An optional token in one palette only invalidates the theme.
  QByteArray base = withReplaced(validJson(), ", \"textInverse\": \"#1A1D21\"",
                                 ", \"textInverse\": \"#1A1D21\", \"pill\": \"#E8EDF2\"");
  Theme t = Theme::loadFromJson(base);
  QVERIFY(!t.isValid);
}

void TstTheme::badVersion() {
  Theme t = Theme::loadFromJson(withReplaced(validJson(), "\"version\": 1", "\"version\": 2"));
  QVERIFY(!t.isValid);
}

void TstTheme::missingTwelfthTokenRejected() {
  //  Both palettes must carry the full 12-token set.
  Theme tLight = Theme::loadFromJson(
    withReplaced(validJson(), ", \"textInverse\": \"#1A1D21\"", ""));
  QVERIFY(!tLight.isValid);
  Theme tDark = Theme::loadFromJson(
    withReplaced(validJson(), ", \"textInverse\": \"#E8EAED\"", ""));
  QVERIFY(!tDark.isValid);
}

void TstTheme::cardTextAccepted() {
  //  Optional per-card text (e.g. outgoing on a saturated bubble) is valid
  //  and resolves; cards without text keep the global text token.
  QByteArray withCardText = withReplaced(
    validJson(), "\"bg2\": \"$bubbleOut2\", \"tail\"",
    "\"bg2\": \"$bubbleOut2\", \"text\": \"$textInverse\", \"tail\"");
  Theme t = Theme::loadFromJson(withCardText);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QCOMPARE(t.cards.value(QStringLiteral("outgoing")).text, QStringLiteral("$textInverse"));
  QCOMPARE(t.resolve(QStringLiteral("$textInverse"), Theme::Mode::Light, false),
           QColor(QStringLiteral("#1A1D21")));
  QCOMPARE(t.resolve(QStringLiteral("$textInverse"), Theme::Mode::Dark, false),
           QColor(QStringLiteral("#E8EAED")));
  QCOMPARE(t.cards.value(QStringLiteral("incoming")).text, QString());
  //  Delegate contract: empty card.text falls back to the global text token.
  QCOMPARE(t.resolve(t.cards.value(QStringLiteral("outgoing")).text, Theme::Mode::Light, false),
           QColor(QStringLiteral("#1A1D21")));
  QCOMPARE(t.resolve(QStringLiteral("$text"), Theme::Mode::Light, false),
           QColor(QStringLiteral("#1A1D21")));
}

void TstTheme::hexInCardsRejected() {
  Theme t = Theme::loadFromJson(
    withReplaced(validJson(), "\"bg\": \"$bubbleIn\"", "\"bg\": \"#FFFFFF\""));
  QVERIFY(!t.isValid);
}

void TstTheme::transparentOutsideSystemRejected() {
  Theme t = Theme::loadFromJson(
    withReplaced(validJson(), "\"incoming\": {\"bg\": \"$bubbleIn\"", "\"incoming\": {\"bg\": \"transparent\""));
  QVERIFY(!t.isValid);
}

void TstTheme::unknownTokenRejected() {
  Theme t = Theme::loadFromJson(
    withReplaced(validJson(), "$bubbleIn2", "$nope"));
  QVERIFY(!t.isValid);
}

void TstTheme::missingLayoutRejected() {
  QByteArray base = validJson();
  //  Cut out layout_request entirely via marker: easier to corrupt the key name.
  Theme t = Theme::loadFromJson(withReplaced(base, "layout_request", "layout_nope"));
  QVERIFY(!t.isValid);
}

void TstTheme::reservedBlocksAcceptedButIgnored() {
  //  The validator accepts linkPreview/quote/code/buttons (L); the S delegate ignores them.
  QByteArray base = withReplaced(validJson(), "{\"type\": \"body\"}",
                                 "{\"type\": \"quote\"}");
  Theme t = Theme::loadFromJson(base);
  QVERIFY2(t.isValid, qPrintable(t.errorString));
  QByteArray base2 = withReplaced(validJson(), "{\"type\": \"body\"}",
                                  "{\"type\": \"linkPreview\"}");
  Theme t2 = Theme::loadFromJson(base2);
  QVERIFY2(t2.isValid, qPrintable(t2.errorString));
}

void TstTheme::badFlagRejected() {
  Theme t = Theme::loadFromJson(withReplaced(validJson(), "isGrouped", "isFoo"));
  QVERIFY(!t.isValid);
}

void TstTheme::tooDeepRejected() {
  //  hbox=1, vbox=2, bubble=3, body=4, + nested body = 5 (>4).
  QByteArray deep = withReplaced(
    validJson(), "{\"type\": \"body\"}",
    "{\"type\": \"bubble\", \"card\": \"incoming\", \"children\": [{\"type\": \"body\"}]}");
  Theme t = Theme::loadFromJson(deep);
  QVERIFY(!t.isValid);
}

void TstTheme::fractionalArrayRejected() {
  //  Pixel arrays are integers: 12.5 would silently truncate via toInt().
  Theme t = Theme::loadFromJson(
    withReplaced(validJson(), "\"radius\": [12, 12, 12, 4]", "\"radius\": [12, 12, 12, 4.5]"));
  QVERIFY(!t.isValid);
}

void TstTheme::resolveTokens() {
  Theme t = Theme::loadFromJson(validJson());
  QVERIFY(t.isValid);
  QCOMPARE(t.resolve(QStringLiteral("$bubbleIn"), Theme::Mode::Light, false),
           QColor(QStringLiteral("#FFFFFF")));
  QCOMPARE(t.resolve(QStringLiteral("$bubbleIn"), Theme::Mode::Dark, false),
           QColor(QStringLiteral("#1F242B")));
  //  System follows systemDark.
  QCOMPARE(t.resolve(QStringLiteral("$text"), Theme::Mode::System, false),
           QColor(QStringLiteral("#1A1D21")));
  QCOMPARE(t.resolve(QStringLiteral("$text"), Theme::Mode::System, true),
           QColor(QStringLiteral("#E8EAED")));
  QCOMPARE(t.resolve(QStringLiteral("transparent"), Theme::Mode::Light, false),
           QColor(Qt::transparent));
}

void TstTheme::availableThemesListsUserAndSkipsBroken() {
  QStandardPaths::setTestModeEnabled(true);
  QString base = StdLocation::userThemeDir();
  QVERIFY(QDir().mkpath(base + QStringLiteral("/Good")));
  QVERIFY(QDir().mkpath(base + QStringLiteral("/Bad")));
  QFile good(base + QStringLiteral("/Good/theme.json"));
  QVERIFY(good.open(QIODevice::WriteOnly));
  good.write(validJson());
  good.close();
  QFile bad(base + QStringLiteral("/Bad/theme.json"));
  QVERIFY(bad.open(QIODevice::WriteOnly));
  bad.write("{broken");
  bad.close();

  QStringList themes = Theme::availableThemes();
  QVERIFY(themes.contains(QStringLiteral("Bubble")));
  QVERIFY(themes.contains(QStringLiteral("Good")));
  QVERIFY(!themes.contains(QStringLiteral("Bad")));
  QCOMPARE(Theme::themeFileFor(QStringLiteral("Good")),
           QDir(base).absoluteFilePath(QStringLiteral("Good/theme.json")));
  QVERIFY(Theme::themeFileFor(QStringLiteral("Nope")).isEmpty());

  QVERIFY(QDir(base + QStringLiteral("/Good")).removeRecursively());
  QVERIFY(QDir(base + QStringLiteral("/Bad")).removeRecursively());
  QStandardPaths::setTestModeEnabled(false);
}

void TstTheme::realFileLoads() {
  QDir themesDir(QString::fromLocal8Bit(LMC_THEME_DIR));
  const QStringList subdirs =
    themesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  QVERIFY2(!subdirs.isEmpty(), "no themes found");
  QVERIFY(subdirs.contains(QStringLiteral("Bubble")));
  for(const QString& sub : subdirs) {
    QFile f(themesDir.absoluteFilePath(sub + QStringLiteral("/theme.json")));
    QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(QStringLiteral("cannot open ") + sub));
    Theme t = Theme::loadFromJson(f.readAll());
    QVERIFY2(t.isValid, qPrintable(sub + QStringLiteral(": ") + t.errorString));
  }
}

QTEST_MAIN(TstTheme)
#include "tst_theme.moc"
