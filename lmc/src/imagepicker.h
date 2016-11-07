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


#ifndef IMAGEPICKER_H
#define IMAGEPICKER_H

#include <QTableWidget>

class lmcImagePicker : public QTableWidget {
	Q_OBJECT

public:
	lmcImagePicker(QWidget *parent, QList<QString>* source, int picSize, int columns, int* selected, int actionIndex);
	~lmcImagePicker();

protected:
	void currentChanged(const QModelIndex& current, const QModelIndex& preious);
	void mouseMoveEvent(QMouseEvent* e);
	void paintEvent(QPaintEvent* e);
	void leaveEvent(QEvent* e);
	
private:
	int* selected;
	int actionIndex;
	int max_col;

	QTableWidgetItem* hoverItem;
};

#endif // IMAGEPICKER_H
