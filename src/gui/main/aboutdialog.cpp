// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include <QFile>
#include "aboutdialog.h"

//	constructor
lmcAboutDialog::lmcAboutDialog(QWidget* parent, Qt::WindowFlags flags)
    : QDialog(parent, flags) {
  ui.setupUi(this);
  //	set minimum size
  layout()->setSizeConstraint(QLayout::SetMinimumSize);
  //	remove the help button from window button group
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  //	Destroy the window when it closes
  setAttribute(Qt::WA_DeleteOnClose, true);
}

lmcAboutDialog::~lmcAboutDialog(void) {
}

void lmcAboutDialog::init(void) {
  setWindowIcon(QIcon(IDR_APPICON));

  pSettings = new lmcSettings();
  setUIText();

  ui.tabWidget->setCurrentIndex(0);
}

void lmcAboutDialog::settingsChanged(void) {
}

void lmcAboutDialog::changeEvent(QEvent* pEvent) {
  switch(pEvent->type()) {
  case QEvent::LanguageChange:
    setUIText();
    break;
  default:
    break;
  }

  QDialog::changeEvent(pEvent);
}

void lmcAboutDialog::setUIText(void) {
  ui.retranslateUi(this);

  QString title = tr("About %1");
  setWindowTitle(title.arg(lmcStrings::appName()));

  ui.lblTitle->setText(lmcStrings::appName() + "\n" IDA_VERSION);
  ui.lblLogoSmall->setPixmap(QPixmap(IDR_LOGOSMALL));
#if defined(QT_NO_DEBUG)
#define DEBUGINFO " "
#else
#define DEBUGINFO " debug"
#endif
  ui.lblQtVersion->setText(QString("Qt %1  %2" DEBUGINFO "\n%3 %4")
                             .arg(QT_VERSION_STR,
                                  QSysInfo::buildCpuArchitecture(),
                                  QSysInfo::productType(),
                                  QSysInfo::productVersion()));

  QString description(lmcStrings::appDesc() + "\n\n");
  description.append(IDA_COPYRIGHT "\n" IDA_DOMAIN);
  ui.lblDescription->setText(description);

  QFile thanks(IDR_THANKSTEXT);
  if(thanks.open(QIODevice::ReadOnly)) {
    ui.txtThanks->setPlainText(QString(thanks.readAll().constData()));
    thanks.close();
  }

  QFile license(IDR_LICENSETEXT);
  if(license.open(QIODevice::ReadOnly)) {
    ui.txtLicense->setPlainText(QString(license.readAll().constData()));
    license.close();
  }
}
