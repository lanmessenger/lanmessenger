// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "apprunner.h"

int main(int argc, char* argv[]) {
  // Production entry point: shared startup, empty pre-exec hook —
  // this binary stays agent-free.
  return lmcRunApplication(argc, argv, {});
}
