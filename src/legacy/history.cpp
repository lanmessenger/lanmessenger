// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
//
// LEGACY: binary messenger.db reader (QDataStream linked list).
// Replaced by: src/storage/historystore.* (Qt Sql + SQLite, lanmessenger.db).
// Delete after: the SQLite release + one successful legacy import cycle
// (HistoryStore renames the legacy file to .bak; no writers remain).
// New code must not use this except the one-shot import in
// HistoryStore::open.
#include "history.h"
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDir>
#include <QDesktopServices>
#include <QSet>
#include <QStandardPaths>
#include "core/settings.h"

QString History::historyFile(void) {
  lmcSettings settings;
  bool sysPath = settings.value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool();
  QString path = QDir::toNativeSeparators(QStandardPaths::writableLocation(
                                            QStandardPaths::AppLocalDataLocation) +
                                          "/" HC_FILENAME);
  if(!sysPath)
    path = settings.value(IDS_HISTORYPATH, path).toString();
  return path;
}

DBHeader History::readHeader(QDataStream* pStream) {
  DBHeader header;

  *pStream >> header.marker;
  *pStream >> header.version;
  *pStream >> header.headerSize;
  *pStream >> header.count;
  *pStream >> header.first;
  *pStream >> header.last;

  return header;
}

bool History::hasDbMarker(const QString& path) {
  if(!QFile::exists(path))
    return false;
  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
    return false;
  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);
  DBHeader header = readHeader(&stream);
  return header.marker.compare(HC_DBMARKER) == 0;
}

QList<MsgInfo> History::getList(void) {
  return getListFrom(historyFile());
}

QString History::getMessage(qint64 offset) {
  return getMessageFrom(historyFile(), offset);
}

QList<MsgInfo> History::getListFrom(const QString& path) {
  QList<MsgInfo> list;

  if(!QFile::exists(path))
    return list;

  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
    return list;

  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);

  DBHeader header = readHeader(&stream);
  if(header.marker.compare(HC_DBMARKER) != 0)
    return list;
  qint64 next = header.first;

  // A corrupt file can chain offsets into a cycle (or past EOF); this runs
  // in HistoryStore::open() on the main thread, so it must always terminate.
  // header.count itself is untrusted: bound the walk by the file size
  // (every record is >= ~20 bytes) and break on a repeated offset.
  const qint64 fileSize = file.size();
  const qint64 maxSteps = fileSize / 16 + 64;
  QSet<qint64> seen;

  QString marker;
  qint64 offset;
  qint64 date;
  QString name;

  qint64 steps = 0;
  while(next != 0) {
    if(next < 0 || next >= fileSize || seen.contains(next) || steps >= maxSteps)
      break;
    seen.insert(next);
    ++steps;
    if(!stream.device()->seek(next))
      break;
    MsgInfo info;
    stream >> marker;
    stream >> next;
    stream >> offset;
    stream >> date;
    stream >> name;
    if(stream.status() != QDataStream::Ok)
      break;
    list.append(MsgInfo(name, QDateTime::fromMSecsSinceEpoch(date), offset));
  }

  return list;
}

QString History::getMessageFrom(const QString& path, qint64 offset) {
  QString data;

  if(!QFile::exists(path))
    return data;

  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
    return data;

  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);

  QString marker;
  QByteArray buffer;

  const qint64 fileSize = file.size();
  if(offset < 0 || offset >= fileSize)
    return data;
  if(!stream.device()->seek(offset))
    return data;
  stream >> marker;
  if(stream.status() != QDataStream::Ok)
    return data;
  if(marker.compare(HC_DTMARKER) != 0)
    return data;
  //  Pre-fix Qt6 wrote 64-bit length, classic is 32-bit.
  qint64 afterMarker = stream.device()->pos();
  quint32 first = 0;
  quint32 second = 0;
  stream >> first >> second;
  if(stream.status() != QDataStream::Ok)
    return data;
  if(first == 0 && second != 0) {
    stream >> buffer;
  } else {
    stream.device()->seek(afterMarker + 4);
    stream >> buffer;
  }

  data = QString::fromUtf8(buffer, buffer.length());

  file.close();
  return data;
}
