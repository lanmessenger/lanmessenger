// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "gui/chat/rowresolver.h"

#include "core/definitions.h"

QString RowResolver::roleForCard(int messageType, bool own) {
  switch(messageType) {
  case MT_Broadcast:
    return QStringLiteral("broadcast");
  case MT_File:
  case MT_Folder:
    return QStringLiteral("request");
  case MT_Failed:
  case MT_Join:
  case MT_Leave:
  case MT_ChatState:
    return QStringLiteral("system");
  default:
    break;
  }
  return own ? QStringLiteral("outgoing") : QStringLiteral("incoming");
}

QString RowResolver::layoutNameForCard(const QString& card, bool grouped) {
  if(grouped && (card == QLatin1String("incoming") || card == QLatin1String("outgoing")))
    return QStringLiteral("layout_") + card + QStringLiteral("_next");
  return QStringLiteral("layout_") + card;
}

bool RowResolver::reservesAvatar(const QString& card, int avatarSize) {
  return card == QLatin1String("incoming") && avatarSize > 0;
}

ResolvedRow RowResolver::resolve(const Theme& theme, const QString& card, bool grouped) {
  ResolvedRow r;
  r.card = card;
  //  Theme defaults first; layout nodes override below.
  r.text = theme.text;
  r.senderColor = theme.header.senderColor;
  r.timeColor = theme.header.timeColor;
  r.bg.kind = Theme::Bg::Kind::Solid;
  r.bg.solid = theme.page;
  r.avatar = reservesAvatar(card, theme.avatar.size);
  r.header = false;
  if(card == QLatin1String("incoming"))
    r.headerItems = QStringList{QStringLiteral("sender"), QStringLiteral("time")};
  else if(card == QLatin1String("outgoing"))
    r.headerItems = QStringList{QStringLiteral("time")};
  //  In-series rows walk their own tree.
  const QString layoutName = layoutNameForCard(card, grouped);
  const Theme::LayoutNode& rootNode = theme.compiledLayout(layoutName);
  if(rootNode.type != Theme::LayoutNode::Type::HBox &&
     rootNode.type != Theme::LayoutNode::Type::VBox)
    return r;
  const Theme::LayoutNode* root = &rootNode;
  if(root->type == Theme::LayoutNode::Type::HBox) {
    r.hboxGap = root->gap;
    r.avatar = false;
    const Theme::LayoutNode* column = nullptr;
    for(const Theme::LayoutNode& c : root->children) {
      if(c.type == Theme::LayoutNode::Type::Avatar)
        r.avatar = reservesAvatar(card, theme.avatar.size);
      else if(c.type == Theme::LayoutNode::Type::VBox && !column)
        column = &c;
    }
    if(!column) {
      r.header = false;
      r.headerItems.clear();
      return r;
    }
    root = column;
  } else {
    r.avatar = false;
  }
  r.vboxGap = root->gap;
  r.header = false;
  r.headerItems.clear();
  for(const Theme::LayoutNode& c : root->children) {
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
      if(c.hasBg)
        r.bg = c.bg;
      for(int i = 0; i < 4; i++)
        r.padding[i] = c.padding[i];
      r.shadowDx = c.shadowDx;
      r.shadowDy = c.shadowDy;
      r.shadowOpacity = c.shadowOpacity;
      r.marginTop = c.marginTop;
      r.maxWidthPct = c.maxWidthPct;
      r.hasBorder = c.hasBorder;
      r.border = c.border;
      for(int i = 0; i < 4; i++)
        r.corners[i] = c.corners[i];
    }
  }
  return r;
}
