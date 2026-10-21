// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include "core/definitions.h"
#include "core/stdlocation.h"
#include "trace.h"

bool lmcTrace::traceMode;
QString lmcTrace::m_fileName;

lmcTrace::lmcTrace(void) {
}

lmcTrace::~lmcTrace(void) {
}

void lmcTrace::init(bool enabled, const QString& logFile) {
  lmcTrace::traceMode = enabled;
  m_fileName = logFile;

  write("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n"
        "         " IDA_TITLE " " IDA_VERSION " application log\n"
        "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-");
}

QString lmcTrace::fileName() {
  return m_fileName;
}

void lmcTrace::write(const QString& string, bool verbose) {
  if(!traceMode || !verbose)
    return;

  QDir logDir(StdLocation::logDir());
  if(!logDir.exists())
    QDir().mkpath(logDir.absolutePath());
  QFile file(m_fileName);
  if(!file.open(QIODevice::Text | QIODevice::Append))
    return;

  QTextStream stream(&file);

  QString timeStamp = "[" + QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz") + "] ";
  QStringList stringList = string.split("\n", Qt::SkipEmptyParts);
  for(int index = 0; index < stringList.count(); index++)
    stream << timeStamp << stringList[index] << "\n";

  file.close();
}
