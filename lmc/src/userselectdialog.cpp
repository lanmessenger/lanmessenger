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


#include "userselectdialog.h"

lmcUserSelectDialog::lmcUserSelectDialog(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	//	remove the help button from window button group
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(btnOK_clicked()));
	connect(ui.tvUserList, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
		this, SLOT(tvUserList_itemChanged(QTreeWidgetItem*, int)));

	parentToggling = false;
	childToggling = false;
}

lmcUserSelectDialog::~lmcUserSelectDialog() {
}

void lmcUserSelectDialog::init(QList<QTreeWidgetItem*>* pContactsList) {
	ui.tvUserList->setIconSize(QSize(16, 16));
    ui.tvUserList->header()->setSectionsMovable(false);
	ui.tvUserList->header()->setStretchLastSection(false);
    ui.tvUserList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	ui.tvUserList->setCheckable(true);

	ui.tvUserList->clear();
	for(int index = 0; index < pContactsList->count(); index++) {
		QTreeWidgetItem* pItem = pContactsList->value(index)->clone();
		pItem->setCheckState(0, Qt::Unchecked);
		for(int childIndex = 0; childIndex < pItem->childCount(); childIndex++)
			pItem->child(childIndex)->setCheckState(0, Qt::Unchecked);
		ui.tvUserList->addTopLevelItem(pItem);
	}
	ui.tvUserList->expandAll();

	//	load settings
	pSettings = new lmcSettings();
	int viewType = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
	ui.tvUserList->setView((UserListView)viewType);

	ui.btnOK->setEnabled(false);

	selectedContacts.clear();
	selectedCount = 0;
	setUIText();
}

void lmcUserSelectDialog::changeEvent(QEvent* pEvent) {
	switch(pEvent->type()) {
	case QEvent::LanguageChange:
		setUIText();
		break;
	default:
		break;
	}

	QWidget::changeEvent(pEvent);
}

void lmcUserSelectDialog::btnOK_clicked(void) {
	selectedContacts.clear();

	for(int index = 0; index < ui.tvUserList->topLevelItemCount(); index++) {
		for(int childIndex = 0; childIndex < ui.tvUserList->topLevelItem(index)->childCount(); childIndex++) {
			QTreeWidgetItem* item = ui.tvUserList->topLevelItem(index)->child(childIndex);
			if(item->checkState(0) == Qt::Checked) {
				QString szUserId = item->data(0, IdRole).toString();
				selectedContacts.append(szUserId);
			}
		}
	}
}

//	event called when the user checks/unchecks a tree item
void lmcUserSelectDialog::tvUserList_itemChanged(QTreeWidgetItem* item, int column) {
	Q_UNUSED(column);

	//	if parent tree item was toggled, update all its children to the same state
	//	if a child tree was toggled, two cases arise:
	//		if all its siblings and it are checked, update its parent to checked
	//		if all its siblings and it are not checked, update its parent to unchecked
	if(item->data(0, TypeRole).toString().compare("Group") == 0 && !childToggling) {
		parentToggling = true;
		for(int index = 0; index < item->childCount(); index++) {
			item->child(index)->setCheckState(0, item->checkState(0));
			item->checkState(0) ? selectedCount++ : selectedCount--;
		}
		parentToggling = false;
	} else if(item->data(0, TypeRole).toString().compare("User") == 0 && !parentToggling) {
		childToggling = true;
		int nChecked = 0;
		QTreeWidgetItem* parent = item->parent();
		for(int index = 0; index < parent->childCount(); index++)
			if(parent->child(index)->checkState(0))
				nChecked++;
		Qt::CheckState check = (nChecked == parent->childCount()) ? Qt::Checked : Qt::Unchecked;
		parent->setCheckState(0, check);
		item->checkState(0) ? selectedCount++ : selectedCount--;
		childToggling = false;
	}

	ui.btnOK->setEnabled((selectedCount > 0));
}

void lmcUserSelectDialog::setUIText(void) {
	ui.retranslateUi(this);

	setWindowTitle(tr("Select Contacts"));
}
