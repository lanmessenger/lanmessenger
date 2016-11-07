/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2011 Dilip Radhakrishnan.
** 
** Contact:  dilipvrk@gmail.com
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


#ifndef TOOLBUTTON_H
#define TOOLBUTTON_H

#include <QStylePainter>
#include <QStyleOption>
#include <QToolButton>

class lmcToolButton : public QToolButton {
public:
	lmcToolButton(QWidget* parent = 0) : QToolButton(parent) {}

protected:
	virtual void paintEvent(QPaintEvent*) {
		QStylePainter p(this);
		QStyleOptionToolButton opt;
		initStyleOption(&opt);
		opt.features &= (~QStyleOptionToolButton::HasMenu);
		p.drawComplexControl(QStyle::CC_ToolButton, opt);
	}
};

#endif //TOOLBUTTON_H