// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef TRACE_H
#define TRACE_H

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include "stdlocation.h"

class lmcTrace {
public:
  lmcTrace(void);
  ~lmcTrace(void);

  static void init(bool enabled, const QString& logFile);
  static void write(const QString& string, bool verbose = true);
  // Trace file from init(); empty when tracing is off.
  static QString fileName();

private:
  static bool traceMode;
  static QString m_fileName;
};

#endif // TRACE_H
