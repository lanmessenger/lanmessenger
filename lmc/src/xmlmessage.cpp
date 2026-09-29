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


#include "xmlmessage.h"

XmlMessage::XmlMessage(void) : QDomDocument() {
	QDomElement root = createElement(XN_ROOT);
	appendChild(root);
	
	QDomElement head = createElement(XN_HEAD);
	root.appendChild(head);

	QDomElement body = createElement(XN_BODY);
	root.appendChild(body);
}

XmlMessage::XmlMessage(const QString& text) : QDomDocument() {
	setContent(text);
}

XmlMessage::~XmlMessage(void) {
}

bool XmlMessage::addHeader(const QString& nodeName, const QString& nodeValue) {
	return addXmlNode(XN_HEAD, nodeName, nodeValue);
}

bool XmlMessage::addData(const QString& nodeName, const QString& nodeValue) {
	return addXmlNode(XN_BODY, nodeName, nodeValue);
}

QString XmlMessage::header(const QString& nodeName) {
	return getXmlNode(XN_HEAD, nodeName);
}

QString XmlMessage::data(const QString& nodeName) {
	return getXmlNode(XN_BODY, nodeName);
}

bool XmlMessage::removeHeader(const QString& nodeName) {
	return removeXmlNode(XN_HEAD, nodeName);
}

bool XmlMessage::removeData(const QString& nodeName) {
	return removeXmlNode(XN_BODY, nodeName);
}

bool XmlMessage::headerExists(const QString& nodeName) {
	return xmlNodeExists(XN_HEAD, nodeName);
}

bool XmlMessage::dataExists(const QString& nodeName) {
	return xmlNodeExists(XN_BODY, nodeName);
}

XmlMessage XmlMessage::clone(void) {
	XmlMessage newMsg;
	newMsg.setContent(toString());
	return newMsg;
}

bool XmlMessage::isValid(void) {
	QDomElement root = documentElement();
	if(root.isNull())
		return false;

	if(root.tagName() == APP_MARKER)
		return true;
	
	return false;
}

bool XmlMessage::addXmlNode(const QString& parentNode, const QString& nodeName, const QString& nodeValue) {
	QDomElement root = documentElement();
	if(root.isNull())
		return false;

	QDomNodeList nodes = root.elementsByTagName(parentNode);
	if(nodes.isEmpty())
		return false;

	QDomElement parent = nodes.at(0).toElement();
	QDomElement element = createElement(nodeName);
	parent.appendChild(element);
	QDomText elementText = createTextNode(nodeValue);
	element.appendChild(elementText);
	return true;
}

QString XmlMessage::getXmlNode(const QString& parentNode, const QString& nodeName) {
	QDomElement root = documentElement();
	if(root.isNull())
		return QString();

	QDomNodeList nodes = root.elementsByTagName(parentNode);
	if(nodes.isEmpty())
		return QString();

	QDomElement parent = nodes.at(0).toElement();
	nodes = parent.elementsByTagName(nodeName);
	if(nodes.isEmpty())
		return QString();

	QDomElement element = nodes.at(0).toElement();
	return element.text();
}

bool XmlMessage::removeXmlNode(const QString& parentNode, const QString& nodeName) {
	QDomElement root = documentElement();
	if(root.isNull())
		return false;

	QDomNodeList nodes = root.elementsByTagName(parentNode);
	if(nodes.isEmpty())
		return false;

	QDomElement parent = nodes.at(0).toElement();
	nodes = parent.elementsByTagName(nodeName);
	if(nodes.isEmpty())
		return false;

	QDomElement element = nodes.at(0).toElement();
	QDomNode newNode = parent.removeChild(element);
	if(newNode.isNull())
		return false;

	return true;
}

bool XmlMessage::xmlNodeExists(const QString& parentNode, const QString& nodeName) {
	QDomElement root = documentElement();
	if(root.isNull())
		return false;

	QDomNodeList nodes = root.elementsByTagName(parentNode);
	if(nodes.isEmpty())
		return false;

	QDomElement parent = nodes.at(0).toElement();
	nodes = parent.elementsByTagName(nodeName);
	if(nodes.isEmpty())
		return false;

	return true;
}