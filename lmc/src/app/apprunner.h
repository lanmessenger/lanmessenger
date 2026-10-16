/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
**
** Contact:  qualiatech@gmail.com
**
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

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

class QCoreApplication;

int lmcRunApplication(int argc, char* argv[],
                      const std::function<void(QCoreApplication&)>& preExecHook);

#endif // LMC_APPRUNNER_H
