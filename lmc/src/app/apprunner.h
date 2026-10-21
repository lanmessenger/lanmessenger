// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#ifndef LMC_APPRUNNER_H
#define LMC_APPRUNNER_H

// Single application entry point shared by the production binary
// (lmc/src/app/main.cpp) and the GUI test binary
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

// Maps a second-instance command line to the IPC command sent to the running
// instance: "term" iff /term is present, "raise" otherwise (unit-tested).
QString remoteCommandFor(const QStringList& arguments);

#endif // LMC_APPRUNNER_H
