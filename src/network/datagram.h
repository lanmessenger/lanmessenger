// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QByteArray>
#include "core/definitions.h"

struct DatagramHeader;

enum DatagramHeaderMember {
  DH_AppId = 0,
  DH_Type,
  DH_UserId,
  DH_Max
};

class Datagram {
public:
  static void addHeader(DatagramType type, QByteArray& baData);
  static bool getHeader(QByteArray& baDatagram, DatagramHeader** ppHeader);
  static QByteArray getData(QByteArray& baDatagram);
};
