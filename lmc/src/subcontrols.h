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


//	This file contains definitions for subclassed controls

#ifndef SUBCONTROLS_H
#define SUBCONTROLS_H

#include <QStylePainter>
#include <QStyleOption>
#include <QToolButton>
#include <QLabel>
#include <QLineEdit>
#include "uidefinitions.h"

class lmcToolButton : public QToolButton {
public:
	lmcToolButton(QWidget* parent = 0);

protected:
	void paintEvent(QPaintEvent*);
};

class lmcLabel : public QLabel {
public:
	lmcLabel(QWidget* parent = 0);

	QString text(void) const;
	void setText(const QString& text);

protected:
	void resizeEvent(QResizeEvent*);
	void paintEvent(QPaintEvent*);

private:
	void setElidedText(void);

	QString actualText;
	QString elidedText;
};

class lmcLineEdit : public QLineEdit {
	Q_OBJECT

public:
	lmcLineEdit(QWidget* parent = 0);

signals:
	void lostFocus(void);

protected:
	void focusInEvent(QFocusEvent* event);
	void focusOutEvent(QFocusEvent* event);
};

#endif //SUBCONTROLS_H
