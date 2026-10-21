// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include "messaging/xmlmessage.h"

class lmcWebNetwork : public QObject {
  Q_OBJECT

public:
  lmcWebNetwork(void);
  ~lmcWebNetwork(void);

  void init(void);
  void start(void);
  void stop(void);
  void sendMessage(QString* lpszUrl, QString* lpszData);
  void settingsChanged(void);

signals:
  void messageReceived(QString* lpszData);

private slots:
  void slotError(QNetworkReply::NetworkError code);
  void replyFinished(QNetworkReply* reply);

private:
  enum ErrorType {
    ET_Busy,
    ET_Error
  };

  void sendMessage(const QUrl& url);
  void raiseError(ErrorType type);

  QNetworkAccessManager* manager;
  bool active;
};
