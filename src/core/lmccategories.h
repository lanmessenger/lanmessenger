// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#pragma once

// Categories from todo/qt-logging-migration.md, section 2. Declared here,
// defined once in core/lmclogging.cpp (a second definition would violate ODR).
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lmcApp)
Q_DECLARE_LOGGING_CATEGORY(lmcNet)
Q_DECLARE_LOGGING_CATEGORY(lmcNetTcp)
Q_DECLARE_LOGGING_CATEGORY(lmcNetUdp)
Q_DECLARE_LOGGING_CATEGORY(lmcMsg)
Q_DECLARE_LOGGING_CATEGORY(lmcFile)
Q_DECLARE_LOGGING_CATEGORY(lmcCore)
Q_DECLARE_LOGGING_CATEGORY(lmcUi)
