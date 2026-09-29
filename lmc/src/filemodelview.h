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


#ifndef FILEMODELVIEW_H
#define FILEMODELVIEW_H

#include <QMetaType>
#include <QPointF>
#include <QVector>
#include <QStyledItemDelegate>
#include <QAbstractListModel>
#include <QWidget>
#include <QDateTime>
#include <QCoreApplication>


/****************************************************************************
**	Class: FileView
**	Description: Takes care of rendering the item
****************************************************************************/
class FileView {
	Q_DECLARE_TR_FUNCTIONS(FileView)

public:
	enum DisplayMode { DM_Normal, DM_Selected, DM_Max };
	enum TransferMode { TM_Send, TM_Receive, TM_Max };
	enum TransferState { TS_Wait, TS_Confirm, TS_Send, TS_Receive, TS_Complete, TS_Decline, TS_Cancel, TS_Abort, TS_Max };
	
	FileView(QString id = QString());

	void paint(QPainter* painter, const QRect& rect, const QPalette& palette, DisplayMode mode) const;
	QSize sizeHint() const;

	QString	id;
    int type;
	QString	filePath;
	QString	fileName;
	qint64	fileSize;
	QString userId;
	QString userName;
	qint64	position;
	qint64	speed;
	QString	fileDisplay;
	QString sizeDisplay;
	QString posDisplay;
	QString speedDisplay;
	QString timeDisplay;
	TransferMode mode;
	TransferState state;
	QPixmap icon;
    QDateTime startTime;
};

QDataStream &operator << (QDataStream &out, const FileView &view);
QDataStream &operator >> (QDataStream &in, FileView &view);

Q_DECLARE_METATYPE(FileView)


/****************************************************************************
**	Class: FileDelegate
**	Description: Delegates rendering and user input
****************************************************************************/
class FileDelegate : public QStyledItemDelegate {
	Q_OBJECT

public:
	FileDelegate(QWidget* parent = 0) : QStyledItemDelegate(parent) {}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
};


/****************************************************************************
**	Class: FileModel
**	Description: Handles the underlying data
****************************************************************************/
class FileModel : public QAbstractListModel {
	Q_OBJECT

public:
	FileModel(QObject* parent = 0) : QAbstractListModel(parent) {}

	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool insertRows(int position, int rows, const QModelIndex& index = QModelIndex());
	bool removeRows(int position, int rows, const QModelIndex& index = QModelIndex());
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::UserRole);
	void insertItem(int position, FileView* fileTransfer);
	void removeItem(int position);
	FileView* item(int position);
	FileView* item(QString id);
	FileView* item(QString id, FileView::TransferMode mode);
	int itemIndex(QString id, FileView::TransferMode mode);
	void itemChanged(int position);
	void loadData(QString filePath);
	void saveData(QString filePath);

private:
	QList<FileView> transferList;
};

#endif // FILEMODELVIEW_H
