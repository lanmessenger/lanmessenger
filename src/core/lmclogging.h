// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

// Qt logging file sink (migration step 1): one handler per process,
// dual-writes to the same file as lmcTrace. No file without /trace.
#include <QString>

class LmcLogging {
public:
  static void init(bool enabled, const QString& logFile);
  static void enable(const QString& logFile);
  static void disable();
  // Path from enable(); empty when the sink is off.
  static QString fileName();
};
