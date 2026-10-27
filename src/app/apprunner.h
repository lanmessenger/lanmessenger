// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

// Single application entry point shared by the production binary
// (src/app/main.cpp) and the GUI test binary
// (tools/lmc-agent/test-main.cpp): same startup, no behavior drift
// between the binary under test and production.
//
// preExecHook runs in the GUI thread after the core starts and before
// exec(). It is deliberately test-agnostic: production passes an empty
// hook (the binary stays agent-free), the test binary starts its
// in-process agent server here.

#include <functional>

#include <QStringList>

class QCoreApplication;

int lmcRunApplication(int argc, char* argv[],
                      const std::function<void(QCoreApplication&)>& preExecHook);

// "term" when /term or /termwait is present (exact element match,
// case-insensitive), "raise" otherwise.
QString remoteCommandFor(const QStringList& arguments);
// True when /termwait is present. Second instance only.
bool wantsTermWait(const QStringList& arguments);
