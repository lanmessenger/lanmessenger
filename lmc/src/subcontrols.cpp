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


#include <QApplication>
#include "lmcstrings.h"
#include "subcontrols.h"

lmcToolButton::lmcToolButton(QWidget* parent) : QToolButton(parent) {
}

void lmcToolButton::paintEvent(QPaintEvent*) {
	QStylePainter p(this);
	QStyleOptionToolButton opt;
	initStyleOption(&opt);
	opt.features &= (~QStyleOptionToolButton::HasMenu);
	p.drawComplexControl(QStyle::CC_ToolButton, opt);
}

lmcLabel::lmcLabel(QWidget* parent) : QLabel(parent) {
	actualText = elidedText = QString();
}

QString lmcLabel::text(void) const {
	return actualText;
}

void lmcLabel::setText(const QString& text) {
	actualText = text;
	setElidedText();
}

void lmcLabel::resizeEvent(QResizeEvent*) {
	setElidedText();
}

void lmcLabel::paintEvent(QPaintEvent*) {
	QStylePainter p(this);
	p.drawText(rect(), alignment(), elidedText);
}

void lmcLabel::setElidedText(void) {
	elidedText = fontMetrics().elidedText(actualText, Qt::ElideRight, width());
}

lmcLineEdit::lmcLineEdit(QWidget* parent) : QLineEdit(parent) {
	QPalette p = QApplication::palette();
	p.setColor(QPalette::Base, p.color(QPalette::Window));
    p.setColor(QPalette::Text, GRAY_TEXT_COLOR);
	setPalette(p);
}

void lmcLineEdit::focusInEvent(QFocusEvent* event) {
	QLineEdit::focusInEvent(event);

	setSelection(0, text().length());

	QPalette p = QApplication::palette();
	p.setColor(QPalette::Base, p.color(QPalette::Base));
	p.setColor(QPalette::Text, p.color(QPalette::Text));
	setPalette(p);
}

void lmcLineEdit::focusOutEvent(QFocusEvent* event) {
	QLineEdit::focusOutEvent(event);

	QPalette p = QApplication::palette();
	p.setColor(QPalette::Base, p.color(QPalette::Window));
    p.setColor(QPalette::Text, GRAY_TEXT_COLOR);
	setPalette(p);

	emit lostFocus();
}
