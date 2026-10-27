// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "webnetwork.h"
#include <QDebug>

const QString ErrorTypeNames[] = {"busy", "error"};

lmcWebNetwork::lmcWebNetwork(void) {
  active = false;
  manager = new QNetworkAccessManager(this);
  connect(manager, &QNetworkAccessManager::finished, this, &lmcWebNetwork::replyFinished);
}

lmcWebNetwork::~lmcWebNetwork(void) {}

void lmcWebNetwork::init(void) {}

void lmcWebNetwork::start(void) {}

void lmcWebNetwork::stop(void) {
  const QList<QNetworkReply*> pending = manager->findChildren<QNetworkReply*>();
  for(QNetworkReply* reply : pending) {
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
  }
  active = false;
}

void lmcWebNetwork::sendMessage(QString* lpszUrl, QString* lpszData) {
  Q_UNUSED(lpszData);

  if(!active)
    sendMessage(QUrl(*lpszUrl));
  else
    raiseError(ET_Busy);
}

void lmcWebNetwork::settingsChanged(void) {}

void lmcWebNetwork::slotError(QNetworkReply::NetworkError code) {
  if(code != QNetworkReply::NoError) {
    raiseError(ET_Error);
    active = false;
  }
}

void lmcWebNetwork::replyFinished(QNetworkReply* reply) {
  if(reply->error() != QNetworkReply::NoError) {
    reply->deleteLater();
    active = false;
    return;
  }

  // check if there was an HTTP redirection
  QVariant redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
  if(!redirect.isNull()) {
    // send a new request to the redirected url
    sendMessage(redirect.toUrl());
  } else {
    // no redirection, get the data from the reply
    QByteArray data = reply->readAll();
    QString szMessage = QString(data.constData());
    emit messageReceived(&szMessage);
    reply->close();
  }

  reply->deleteLater();
  active = false;
}

void lmcWebNetwork::sendMessage(const QUrl& url) {
  if(url.isEmpty()) {
    raiseError(ET_Error);
    return;
  }

  active = true;

  QNetworkReply* reply = manager->get(QNetworkRequest(url));
  connect(reply, &QNetworkReply::errorOccurred, this, &lmcWebNetwork::slotError);
}

void lmcWebNetwork::raiseError(ErrorType type) {
  XmlMessage xmlMessage;
  xmlMessage.addHeader(XN_TYPE, MessageTypeNames[MT_WebFailed]);
  xmlMessage.addData(XN_ERROR, ErrorTypeNames[type]);

  QString szMessage = xmlMessage.toString();
  emit messageReceived(&szMessage);
}
