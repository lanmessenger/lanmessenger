#include <QApplication>
#include <QDir>
#include <QPixmap>
#include <QRandomGenerator>
#include <QResource>
#include <QTimer>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QUdpSocket>
#include <QtTest>

#include "gui/main/aboutdialog.h"
#include "application.h"
#include "gui/history/historywindow.h"
#include "app/lmc.h"
#include "gui/main/mainwindow.h"
#include "core/settings.h"
#include "core/stdlocation.h"
#include "gui/files/transferwindow.h"

namespace {

const QString e2eAppId = QStringLiteral("lmc-e2e-test");

QString artifactsDir(void) {
  const QString dir = qEnvironmentVariable("LMC_E2E_ARTIFACTS_DIR");
  return dir.isEmpty() ? QStringLiteral("e2e-artifacts") : dir;
}

int findFreePort(void) {
  for(int attempt = 0; attempt < 32; attempt++) {
    int port = 30000 + QRandomGenerator::global()->bounded(20000);
    QTcpServer tcp;
    QUdpSocket udp;
    if(tcp.listen(QHostAddress::AnyIPv4, quint16(port)) &&
       udp.bind(QHostAddress::AnyIPv4, quint16(port)))
      return port;
  }
  return -1;
}

template <typename T>
T* findWindow(void) {
  const QWidgetList widgets = QApplication::topLevelWidgets();
  for(int index = 0; index < widgets.count(); index++) {
    T* typed = qobject_cast<T*>(widgets.at(index));
    if(typed)
      return typed;
  }
  return nullptr;
}

bool saveShot(QWidget* widget, const QString& name) {
  if(!QDir().mkpath(artifactsDir()))
    return false;
  const QPixmap shot = widget->grab();
  return !shot.isNull() && shot.save(artifactsDir() + QLatin1Char('/') + name);
}

bool triggerShortcut(QWidget* target, const QKeySequence& sequence) {
  if(sequence.count() == 0)
    return false;
  const QKeyCombination combo = sequence[0];
  QTest::keyClick(target, combo.key(), combo.keyboardModifiers());
  return true;
}

} // namespace

class TstLmcGui : public QObject {
  Q_OBJECT

private slots:
  void initTestCase(void);
  void mainWindowAppears(void);
  void openFileTransfers(void);
  void openHistory(void);
  void openAboutDialog(void);
  void cleanupTestCase(void);

private:
  lmcCore* pCore = nullptr;
  bool aboutShotDone = false;
  bool aboutShotOk = false;
};

void TstLmcGui::initTestCase(void) {
  QDir().mkpath(artifactsDir());

  {
    lmcSettings settings;
    settings.setValue(IDS_VERSION, IDA_VERSION);
    settings.setValue(IDS_AUTOSHOW, true);
    settings.setValue(IDS_USERNAME, QStringLiteral("E2E Tester"));
    settings.sync();
  }

  const int port = findFreePort();
  QVERIFY2(port > 0, "could not bind a free UDP/TCP port for the instance");

  pCore = new lmcCore();
  const QString args = QStringLiteral("/silent\n/loopback\n/port=%1\n").arg(port);
  pCore->init(args);
  QVERIFY(pCore->receiveAppMessage(args + QStringLiteral("/new\n")));
  QVERIFY(pCore->start());
}

void TstLmcGui::mainWindowAppears(void) {
  lmcMainWindow* window = findWindow<lmcMainWindow>();
  QVERIFY2(window, "main window was not created");
  QVERIFY(QTest::qWaitForWindowExposed(window));
  QCOMPARE(window->windowTitle(), lmcStrings::appName());
  QVERIFY2(saveShot(window, QStringLiteral("01-main-window.png")), "main window screenshot failed");
}

void TstLmcGui::openFileTransfers(void) {
  lmcMainWindow* window = findWindow<lmcMainWindow>();
  QVERIFY(window);
  window->activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(window));

  QVERIFY(triggerShortcut(window, QKeySequence(Qt::CTRL | Qt::Key_J)));

  QTRY_VERIFY_WITH_TIMEOUT(findWindow<lmcTransferWindow>() != nullptr, 5000);
  lmcTransferWindow* transfers = findWindow<lmcTransferWindow>();
  QVERIFY(QTest::qWaitForWindowExposed(transfers));
  QCOMPARE(transfers->windowTitle(), QStringLiteral("File Transfers"));
  QVERIFY2(saveShot(transfers, QStringLiteral("02-file-transfers.png")), "transfers screenshot failed");
}

void TstLmcGui::openHistory(void) {
  lmcMainWindow* window = findWindow<lmcMainWindow>();
  QVERIFY(window);
  window->activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(window));

  QVERIFY(triggerShortcut(window, QKeySequence(Qt::CTRL | Qt::Key_H)));

  QTRY_VERIFY_WITH_TIMEOUT(findWindow<lmcHistoryWindow>() != nullptr, 5000);
  lmcHistoryWindow* history = findWindow<lmcHistoryWindow>();
  QVERIFY(QTest::qWaitForWindowExposed(history));
  QCOMPARE(history->windowTitle(), QStringLiteral("Message History"));
  QVERIFY2(saveShot(history, QStringLiteral("03-message-history.png")), "history screenshot failed");
}

void TstLmcGui::openAboutDialog(void) {
  lmcMainWindow* window = findWindow<lmcMainWindow>();
  QVERIFY(window);
  window->activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(window));

  // showAbout() runs the dialog modally, so arm the capture first: it fires
  // inside the nested event loop, saves the shot and closes the dialog.
  aboutShotDone = false;
  aboutShotOk = false;
  QTimer::singleShot(2000, this, [this]() {
    lmcAboutDialog* about = findWindow<lmcAboutDialog>();
    if(about && QTest::qWaitForWindowExposed(about))
      aboutShotOk = saveShot(about, QStringLiteral("04-about-dialog.png"));
    aboutShotDone = true;
    if(about)
      about->close();
  });
  QMetaObject::invokeMethod(window, "trayAboutAction_triggered", Qt::DirectConnection);

  QTRY_VERIFY_WITH_TIMEOUT(aboutShotDone, 10000);
  QVERIFY2(aboutShotOk, "about dialog screenshot failed");
  QTRY_VERIFY_WITH_TIMEOUT(findWindow<lmcAboutDialog>() == nullptr, 5000);
}

void TstLmcGui::cleanupTestCase(void) {
  if(pCore) {
    QMetaObject::invokeMethod(pCore, "aboutToExit", Qt::DirectConnection);
    delete pCore;
    pCore = nullptr;
  }
}

int main(int argc, char* argv[]) {
  QTemporaryDir homeDir;
  if(!homeDir.isValid())
    return 1;
  const QString home = homeDir.path();
  qputenv("HOME", home.toLocal8Bit());
  qputenv("USERPROFILE", home.toLocal8Bit());
  qputenv("APPDATA", (home + QStringLiteral("/AppData/Roaming")).toLocal8Bit());
  qputenv("LOCALAPPDATA", (home + QStringLiteral("/AppData/Local")).toLocal8Bit());
  qputenv("XDG_CONFIG_HOME", (home + QStringLiteral("/.config")).toLocal8Bit());
  qputenv("XDG_DATA_HOME", (home + QStringLiteral("/.local/share")).toLocal8Bit());
  qputenv("XDG_CACHE_HOME", (home + QStringLiteral("/.cache")).toLocal8Bit());

  Application application(e2eAppId, argc, argv);

  QString appDir = qEnvironmentVariable("LMC_APP_DIR");
  if(appDir.isEmpty())
    appDir = QCoreApplication::applicationDirPath();
  QDir::setCurrent(appDir);
  QResource::registerResource(StdLocation::resourceFile());

  QApplication::setApplicationName(IDA_PRODUCT);
  QApplication::setOrganizationName(IDA_COMPANY);
  QApplication::setOrganizationDomain(IDA_DOMAIN);

  Application::loadTranslations(StdLocation::resLangDir());
  Application::loadTranslations(StdLocation::sysLangDir());
  Application::loadTranslations(StdLocation::userLangDir());

  TstLmcGui test;
  return QTest::qExec(&test, argc, argv);
}

#include "tst_lmc_gui.moc"
