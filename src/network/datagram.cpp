// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QDataStream>
#include "core/trace.h"
#include "datagram.h"

void Datagram::addHeader(DatagramType type, QByteArray& baData) {
  QByteArray datagramType = DatagramTypeNames[type].toLocal8Bit();
  baData.insert(0, datagramType);
}

bool Datagram::getHeader(QByteArray& baDatagram, DatagramHeader** ppHeader) {
  QString datagramType(baDatagram.mid(0, 6)); // first 6 bytes represent datagram type
  int type = Helper::indexOf(DatagramTypeNames, DT_Max, datagramType);
  if(type < 0)
    return false;

  *ppHeader = new DatagramHeader(
    (DatagramType)type,
    QString(),
    QString());
  return true;
}

QByteArray Datagram::getData(QByteArray& baDatagram) {
  if(baDatagram.length() > 6)
    return baDatagram.mid(6);

  return QByteArray();
}
