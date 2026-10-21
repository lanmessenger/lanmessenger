// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef LMCSTRINGS_H
#define LMCSTRINGS_H

#include <QStringList>
#include <QCoreApplication>

class lmcStrings {
  Q_DECLARE_TR_FUNCTIONS(lmcStrings)

public:
  lmcStrings(void);
  ~lmcStrings(void);

  static void retranslate(void);
  static const QString appName(void);
  static const QString appDesc(void);
  static const QString autoConn(void);
  static const QStringList fontSize(void);
  static const QStringList statusDesc(void);
  static const QStringList soundDesc(void);
  static const QStringList awayTimeDesc(void);
  static const QStringList userListView(void);

private:
  static QString m_appName;
  static QString m_appDesc;
  static QString m_autoConn;
  static QStringList m_fontSize;
  static QStringList m_statusDesc;
  static QStringList m_soundDesc;
  static QStringList m_awayTimeDesc;
  static QStringList m_userListView;
};

#endif // LMCSTRINGS_H
