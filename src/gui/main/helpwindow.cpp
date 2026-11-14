// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2010 - 2012 Qualia Digital Solutions.
// Copyright (c) 2026 LAN Messenger.
#include "helpwindow.h"
#include <QGuiApplication>
#include <QScreen>
#include <QKeyEvent>
#include "core/settings.h"

lmcHelpWindow::lmcHelpWindow(QRect* pRect, QWidget* parent)
    : QWidget(parent) {
  ui.setupUi(this);

  //	Destroy the window when it closes
  setAttribute(Qt::WA_DeleteOnClose, true);

  move(pRect->center() - rect().center());
  QRect screenRect = QGuiApplication::primaryScreen()->geometry();
  if(!screenRect.contains(geometry(), true)) {
    QRect windowRect = geometry();
    if(windowRect.right() > screenRect.right())
      windowRect.translate(screenRect.right() - windowRect.right(), 0);
    if(windowRect.left() < screenRect.left())
      windowRect.translate(qAbs(windowRect.left() - screenRect.left()), 0);
    if(windowRect.bottom() > screenRect.bottom())
      windowRect.translate(0, screenRect.bottom() - windowRect.bottom());
    if(windowRect.top() < screenRect.top())
      windowRect.translate(0, qAbs(windowRect.top() - screenRect.top()));
    setGeometry(windowRect);
  }

  ui.txtHelp->installEventFilter(this);
  ui.btnClose->installEventFilter(this);
}

lmcHelpWindow::~lmcHelpWindow() {
}

void lmcHelpWindow::init(void) {
  setWindowIcon(QIcon(IDR_APPICON));

  pSettings = new lmcSettings();
  restoreGeometry(pSettings->value(IDS_WINDOWHELP).toByteArray());
  setUIText();
}

void lmcHelpWindow::stop(void) {
  pSettings->setValue(IDS_WINDOWHELP, saveGeometry());
}

void lmcHelpWindow::settingsChanged(void) {
}

bool lmcHelpWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
  Q_UNUSED(pObject);
  if(pEvent->type() == QEvent::KeyPress) {
    QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(pEvent);
    if(pKeyEvent->key() == Qt::Key_Escape) {
      close();
      return true;
    }
  }

  return false;
}

void lmcHelpWindow::changeEvent(QEvent* pEvent) {
  switch(pEvent->type()) {
  case QEvent::LanguageChange:
    setUIText();
    break;
  default:
    break;
  }

  QWidget::changeEvent(pEvent);
}

void lmcHelpWindow::setUIText(void) {
  ui.retranslateUi(this);

  setWindowTitle(tr("Help"));

  const QString docsLink = QStringLiteral(IDA_DOCS_URL);
#if defined Q_OS_WIN
  const QString networkLink = docsLink + QStringLiteral("/install-windows#firewall");
#elif defined Q_OS_MAC
  const QString networkLink = docsLink + QStringLiteral("/install-macos");
#else
  const QString networkLink = docsLink + QStringLiteral("/install-linux#firewall");
#endif
  const QString issuesLink = QStringLiteral(IDA_ISSUES_URL);
  const QString repoLink = QStringLiteral(IDA_REPO_URL);

  QString html = tr(
    "<html>"
    "<head>"
    "<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />"
    "</head>"
    "<body>"
    "<h2>Welcome to LAN Messenger Help</h2>"
    "<h3>Documentation</h3>"
    "<p>"
    "You can find guides and references in the <a href='%1'>documentation</a>."
    "</p>"
    "<p>"
    "No peers in the list? Check the <a href='%2'>network notes for your system</a>."
    "</p>"
    "<h3>Report a problem</h3>"
    "<p>"
    "Found a bug on your subnet? Please <a href='%3'>open an issue</a> and include "
    "your operating system, app version, and network setup."
    "</p>"
    "<h3>Contribute to the project</h3>"
    "<p>"
    "You can help the LAN Messenger project giving some of your time to help the community, to translate the application or documentation, and to test the program. And of course, you can <b>promote</b> LAN Messenger."
    "</p>"
    "<p>"
    "If you find LAN Messenger useful, please <a href='%4'>star us on GitHub</a>."
    "</p>"
    "</body>"
    "</html>");
  ui.txtHelp->setHtml(html.arg(docsLink, networkLink, issuesLink, repoLink));
}
