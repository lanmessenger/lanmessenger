// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVector>

//  Native chat render: literal colors and rect metrics on
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
  //  metrics section exists. Per-node keys: bg, padding [T,R,B,L],
  //  shadow [dx,dy,blur,opacity], marginTop (row top gap, int >= 0),
  //  maxWidthPct. All optional: absent bg = transparent, absent
  //  padding/shadow/marginTop = zeros (no padding, no shadow, no top gap),
  //  absent maxWidthPct = 100 (full width).
  //  Rows are tailless: no tail
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
    Kind kind = Kind::Transparent;
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
    //  Literal colors, empty/absent = theme default (see Header/text below).
    DualColor senderColor; // header only
    bool hasSenderColor = false;
    DualColor timeColor; // header only
    bool hasTimeColor = false;
    DualColor textColor; // body/fileCard only
    bool hasTextColor = false;
    DualColor border; // body/fileCard only
    bool hasBorder = false;
    Bg bg; // body/fileCard only, absent = transparent
    bool hasBg = false;
    //  Per-corner radii [TL, TR, BR, BL], body/fileCard only, optional.
    //  Absent = square. Outgoing nodes carry hand-mirrored values,
    //  no code mirror exists.
    int corners[4] = {0, 0, 0, 0};
    bool hasCorners = false;
    //  Rect metrics, body/fileCard only, all optional: absent bg =
    //  transparent, absent padding/shadow/marginTop = zeros, absent
    //  maxWidthPct = 100 (full width). The row top gap
    //  is per tree: series-start trees carry the wide gap, _next trees
    //  the dense in-series one.
    int padding[4] = {0, 0, 0, 0}; // [T, R, B, L]
    double shadowDx = 0.0;
    double shadowDy = 0.0;
    double shadowBlur = 0.0;
    double shadowOpacity = 0.0;
    int marginTop = 0;
    int maxWidthPct = 100;
    QVector<LayoutNode> children;
  };

  struct Header {
    int senderWeight = 600;
    //  Theme-level header colors are defaults; layout header nodes override.
    QString timeFormat = QStringLiteral("HH:mm");
    bool outgoingYouLabel = false;
    //  Sizes scale the local display size.
    //  senderWeight styles the incoming name only ("You" and time are regular).
    double senderSize = 1.0;
    double timeSize = 1.0;
    DualColor senderColor;
    DualColor timeColor;
  };

  bool isValid = false;
  QString errorString;
  //  UI label; empty = ID fallback.
  QString displayName;
  View view;
  Avatar avatar;
  Header header;
  //  Single-use global colors: default body text, links, view fill.
  DualColor text;
  DualColor link;
  DualColor page;
  //  Raw layout trees (kept for L) and the compiled engine input.
  //  Each layout_* is a single root object (hbox/vbox), not an array.
  QMap<QString, QJsonObject> layouts;
  QMap<QString, LayoutNode> compiled;

  static Theme loadFromJson(const QByteArray& data);
  static Theme loadFromFile(const QString& filePath);
  static Theme builtInDefault();
  static QString defaultJsonText();
  //  Theme discovery: validated names, user dir overrides built-in qrc.
  static QStringList availableThemes();
  //  Resolve a theme name to its <Name>.json (empty when unknown).
  static QString themeFileFor(const QString& name);
  //  Display label for Preferences: JSON displayName or the ID fallback.
  static QString displayNameFor(const QString& name);

  //  Per-mode picks over compiled literals: a mode switch needs
  //  no recompile.
  static QColor pick(const DualColor& c, bool darkMode);
  static void pickBg(const Bg& bg, bool darkMode, QColor& first, QColor& second);
  //  Mode::System resolves via systemDark; Light/Dark ignore it.
  static bool isDark(Mode mode, bool systemDark);
  QJsonObject layoutFor(const QString& name) const;
  //  Compiled node for layout_name (every valid theme carries all seven).
  const LayoutNode& compiledLayout(const QString& name) const;
};
