// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "history.h"
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDir>
#include <QDesktopServices>
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

void History::create(QString path) {
  QFile file(path);
  if(!file.open(QIODevice::ReadWrite))
    return;

  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);

  DBHeader header(HC_DBMARKER, HC_HDRSIZE, HC_VERSION, 0, 0, 0);
  writeHeader(&stream, &header);

  file.close();
}

void History::writeHeader(QDataStream* pStream, DBHeader* pHeader) {
  pStream->device()->seek(0);

  *pStream << pHeader->marker;
  *pStream << pHeader->version;
  *pStream << pHeader->headerSize;
  *pStream << pHeader->count;
  *pStream << pHeader->first;
  *pStream << pHeader->last;
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

int History::save(QString user, QDateTime date, QString* lpszData) {
  QString path = historyFile();

  QDir dir = QFileInfo(path).dir();
  if(!dir.exists())
    dir.mkpath(dir.absolutePath());

  if(!QFile::exists(path))
    create(path);

  QFile file(path);
  if(!file.open(QIODevice::ReadWrite))
    return -1;

  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);

  DBHeader header = readHeader(&stream);
  if(header.marker.compare(HC_DBMARKER) != 0) {
    file.close();
    return -1;
  }

  qint64 dataPos = insertData(&stream, lpszData);
  qint64 newIndex = insertIndex(&stream, dataPos, user, date);
  updateIndex(&stream, header.last, newIndex);

  header.count++;
  header.first = (header.first == 0) ? newIndex : header.first;
  header.last = newIndex;

  writeHeader(&stream, &header);

  file.close();
  return 0;
}

qint64 History::insertData(QDataStream* pStream, QString* lpszData) {
  qint64 lastPos = pStream->device()->size();
  pStream->device()->seek(lastPos);

  QByteArray data = lpszData->toUtf8();

  *pStream << QString(HC_DTMARKER);
  //  qint32: QByteArray::length() is 64-bit qsizetype since Qt6.
  *pStream << static_cast<qint32>(data.size());
  *pStream << data;

  return lastPos;
}

qint64 History::insertIndex(QDataStream* pStream, qint64 dataPos, QString user, QDateTime date) {
  qint64 lastPos = pStream->device()->size();
  pStream->device()->seek(lastPos);

  qint64 nextPos = 0;

  *pStream << QString(HC_IDMARKER);
  *pStream << nextPos;
  *pStream << dataPos;
  *pStream << date.toMSecsSinceEpoch();
  *pStream << user;

  return lastPos;
}

void History::updateIndex(QDataStream* pStream, qint64 oldIndex, qint64 newIndex) {
  pStream->device()->seek(oldIndex);

  *pStream << QString(HC_IDMARKER);
  *pStream << newIndex;
}

QList<MsgInfo> History::getList(void) {
  QList<MsgInfo> list;

  lmcSettings settings;
  QString path = historyFile();

  if(!QFile::exists(path))
    return list;

  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
    return list;

  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);

  DBHeader header = readHeader(&stream);
  qint64 next = header.first;

  QString marker;
  qint64 offset;
  qint64 date;
  QString name;

  while(next != 0) {
    stream.device()->seek(next);
    MsgInfo info;
    stream >> marker;
    stream >> next;
    stream >> offset;
    stream >> date;
    stream >> name;
    list.append(MsgInfo(name, QDateTime::fromMSecsSinceEpoch(date), offset));
  }

  return list;
}

QString History::getMessage(qint64 offset) {
  QString data;

  lmcSettings settings;
  QString path = historyFile();

  if(!QFile::exists(path))
    return data;

  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
    return data;

  QDataStream stream(&file);
  stream.setVersion(QDataStream::Qt_5_15);

  QString marker;
  QByteArray buffer;

  stream.device()->seek(offset);
  stream >> marker;
  //  Pre-fix Qt6 wrote 64-bit length, classic is 32-bit.
  qint64 afterMarker = stream.device()->pos();
  quint32 first = 0;
  quint32 second = 0;
  stream >> first >> second;
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
