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


#ifndef WEBNETWORK_H
#define WEBNETWORK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include "xmlmessage.h"

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
	enum ErrorType{ET_Busy, ET_Error};

	void sendMessage(const QUrl& url);
	void raiseError(ErrorType type);

	QNetworkAccessManager* manager;
	bool active;
};

#endif // WEBNETWORK_H
