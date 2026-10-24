// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QColor>
#include <QJsonArray>
#include <QMap>
#include <QString>
#include <QVector>

//  Native chat render (Bubble): literal colors and rect metrics on
//  layout_* body/fileCard nodes. Every layout_* resolves into a row:
//  incoming/outgoing take gaps/header/colors/corners/metrics from nodes,
//  with layout_incoming_next/layout_outgoing_next carrying the in-series
//  look (no header/avatar, dense gaps); system/broadcast/request resolve
//  to the same look from their own nodes (no header/avatar there).
//  Colors are literals in three forms: "#hex" (both modes),
//  ["light", "dark"], ["gradient(#from, #to)", "gradient(#from, #to)"].
class Theme {
public:
  enum class Mode {
    System,
    Light,
    Dark
  };

  //  Rect metrics live on layout body/fileCard nodes; no theme-level
  //  metrics section exists. Per-node keys: padding [T,R,B,L],
  //  shadow [dx,dy,blur,opacity], margin (row top gap, int >= 0),
  //  maxWidthPct. Rows are tailless: no tail
  //  triangle is drawn.
  struct View {
    int padding[4] = {0, 8, 8, 8};
  };

  struct Avatar {
    int size = 40;
    QString shape = QStringLiteral("circle");
  };

  //  Per-mode color pair, resolved at paint time so a mode switch
  //  needs no recompile.
  struct DualColor {
    QColor light;
    QColor dark;
  };

  //  Rect fill: solid per-mode color, per-mode gradient (top to bottom),
  //  or transparent (system look).
  struct Bg {
    enum class Kind {
      Solid,
      Gradient,
      Transparent
    };
    Kind kind = Kind::Solid;
    DualColor solid;
    QColor lightFrom, lightTo, darkFrom, darkTo;
  };

  //  Compiled layout node: validated JSON frozen into structs once per theme,
  //  so the per-row path never touches QJson.
  struct LayoutNode {
    enum class Type {
      HBox,
      VBox,
      Avatar,
      Header,
      Body,
      FileCard,
      Reserved
    };
    Type type = Type::Reserved;
    int gap = 0;             // containers only, required in JSON
    QStringList headerItems; // header only, subset of sender/time
    QString ifFlag;          // visibility condition, empty = always
    QString ifNotFlag;
    //  Literal colors, empty/absent = theme default (see Header/text below).
    DualColor senderColor; // header only
    bool hasSenderColor = false;
    DualColor timeColor; // header only
    bool hasTimeColor = false;
    DualColor textColor; // body/fileCard only
    bool hasTextColor = false;
    DualColor border; // body/fileCard only
    bool hasBorder = false;
    Bg bg; // body/fileCard only, required in JSON
    //  Per-corner radii [TL, TR, BR, BL], body/fileCard only.
    //  Outgoing nodes carry hand-mirrored values, no code mirror exists.
    int corners[4] = {12, 12, 12, 4};
    bool hasCorners = false;
    //  Rect metrics, body/fileCard only, required in JSON. The row top gap
    //  is per tree: series-start trees carry the wide gap, _next trees
    //  the dense in-series one.
    int padding[4] = {0, 0, 0, 0}; // [T, R, B, L]
    double shadowDx = 0.0;
    double shadowDy = 0.0;
    double shadowBlur = 0.0;
    double shadowOpacity = 0.0;
    int margin = 0;
    int maxWidthPct = 75;
    QVector<LayoutNode> children;
  };

  struct Header {
    bool senderBold = true;
    //  Theme-level header colors are defaults; layout header nodes override.
    QString timeFormat = QStringLiteral("HH:mm");
    bool outgoingYouLabel = false;
    //  Sizes scale the local display size.
    //  senderBold styles the incoming name only ("You" and time are regular).
    double senderSize = 1.0;
    double timeSize = 1.0;
    DualColor senderColor;
    DualColor timeColor;
  };

  bool isValid = false;
  QString errorString;
  View view;
  Avatar avatar;
  Header header;
  //  Single-use global colors: default body text, links, view fill.
  DualColor text;
  DualColor link;
  DualColor page;
  //  Raw layout trees (kept for L) and the compiled engine input.
  QMap<QString, QJsonArray> layouts;
  QMap<QString, QVector<LayoutNode>> compiled;

  static Theme loadFromJson(const QByteArray& data);
  static Theme loadFromFile(const QString& filePath);
  static Theme builtInDefault();
  static QString defaultJsonText();
  //  The last-resort literal (used when even the compiled-in resource
  //  is unreadable). Exposed so tests cover it: an untested fallback
  //  once shipped stale nodes failing its own validator.
  static QByteArray fallbackJsonText();
  //  Theme discovery: validated names, user dir overrides built-in qrc.
  static QStringList availableThemes();
  //  Resolve a theme name to its theme.json (empty when unknown).
  static QString themeFileFor(const QString& name);

  //  Per-mode picks over compiled literals: a mode switch needs
  //  no recompile.
  static QColor pick(const DualColor& c, bool darkMode);
  static void pickBg(const Bg& bg, bool darkMode, QColor& first, QColor& second);
  //  Mode::System resolves via systemDark; Light/Dark ignore it.
  static bool isDark(Mode mode, bool systemDark);
  QJsonArray layoutFor(const QString& name) const;
  //  Compiled nodes for layout_name (every valid theme carries all seven).
  QVector<LayoutNode> compiledLayout(const QString& name) const;
};
