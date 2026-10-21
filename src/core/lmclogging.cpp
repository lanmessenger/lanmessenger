// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "core/lmclogging.h"
#include "core/lmccategories.h"
#include "core/stdlocation.h"
#include "core/definitions.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <cstdio>

Q_LOGGING_CATEGORY(lmcApp, "lmc.app")
Q_LOGGING_CATEGORY(lmcNet, "lmc.net")
Q_LOGGING_CATEGORY(lmcNetTcp, "lmc.net.tcp")
Q_LOGGING_CATEGORY(lmcNetUdp, "lmc.net.udp")
Q_LOGGING_CATEGORY(lmcMsg, "lmc.msg")
Q_LOGGING_CATEGORY(lmcFile, "lmc.file")
Q_LOGGING_CATEGORY(lmcCoreLog, "lmc.core")
Q_LOGGING_CATEGORY(lmcUi, "lmc.ui")

namespace {
QMutex g_mutex;
QFile g_file;
QString g_fileName;
QtMessageHandler g_previous = nullptr;
bool g_installed = false;

void lmcMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
  // Respect a user-supplied QT_MESSAGE_PATTERN: format only via qFormatLogMessage.
  // No logging from inside the handler (Qt ignores it to prevent recursion).
  const QString formatted = qFormatLogMessage(type, context, message);
  QtMessageHandler previous = nullptr;
  {
    QMutexLocker locker(&g_mutex);
    previous = g_previous;
    if(g_file.isOpen()) {
      QTextStream stream(&g_file);
      stream << formatted << "\n";
      if(type == QtCriticalMsg || type == QtFatalMsg)
        g_file.flush();
    }
  }
  // Forward to keep stderr output (file + console dual-write).
  if(previous) {
    previous(type, context, message);
    return;
  }
  std::fprintf(stderr, "%s\n", formatted.toLocal8Bit().constData());
  std::fflush(stderr);
}
} // namespace

void LmcLogging::enable(const QString& logFile) {
  // User env wins over code: apply built-in rules/pattern only when the
  // corresponding variable is unset. setFilterRules takes '\n'-separated
  // rules only (';' is malformed there; verified on Qt 6.4).
  if(qgetenv("QT_LOGGING_RULES").isEmpty())
    QLoggingCategory::setFilterRules(QStringLiteral("*.critical=true\n*.warning=true\nlmc.*.debug=true"));
  if(qgetenv("QT_MESSAGE_PATTERN").isEmpty())
    qSetMessagePattern(QStringLiteral("[%{time yyyy.MM.dd hh:mm:ss.zzz} %{threadid} %{category}] %{message}"));

  QDir().mkpath(StdLocation::logDir());
  {
    QMutexLocker locker(&g_mutex);
    if(g_file.isOpen())
      g_file.close();
    g_fileName = logFile;
    g_file.setFileName(logFile);
    if(!g_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
      g_fileName.clear();
      return;
    }
    // Legacy file header: timestamp-prefixed like any other line.
    const QString timeStamp =
      QStringLiteral("[") +
      QDateTime::currentDateTime().toString(QStringLiteral("yyyy.MM.dd hh:mm:ss.zzz")) +
      QStringLiteral("] ");
    QTextStream stream(&g_file);
    stream << timeStamp << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n"
           << timeStamp << "         " IDA_TITLE " " IDA_VERSION " application log\n"
           << timeStamp << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
  }
  if(!g_installed) {
    g_previous = qInstallMessageHandler(lmcMessageHandler);
    g_installed = true;
  }
}

void LmcLogging::disable() {
  {
    QMutexLocker locker(&g_mutex);
    if(g_file.isOpen())
      g_file.close();
    g_fileName.clear();
  }
  // Debug is opt-in at runtime: silence it unless the user overrides
  // via QT_LOGGING_RULES (same env-wins rule as enable(); /trace
  // re-enables debug through its own rules there).
  if(qgetenv("QT_LOGGING_RULES").isEmpty())
    QLoggingCategory::setFilterRules(QStringLiteral("*.debug=false"));
  if(g_installed) {
    qInstallMessageHandler(g_previous);
    g_previous = nullptr;
    g_installed = false;
  }
}

void LmcLogging::init(bool enabled, const QString& logFile) {
  if(enabled && !logFile.isEmpty())
    enable(logFile);
  else
    disable();
}

QString LmcLogging::fileName() {
  QMutexLocker locker(&g_mutex);
  return g_fileName;
}
