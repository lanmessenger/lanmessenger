// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.

#include <QtTest>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTemporaryDir>
#include <QTextEdit>

#include "core/definitions.h"
#include "core/shared.h"
#include "gui/chat/chatwindow.h"
#include "messaging/xmlmessage.h"

namespace {

User makeUser(const QString& id, const QString& name, uint caps) {
  return User(id, QStringLiteral("1.0"), QStringLiteral("127.0.0.1"), name, QStringLiteral("chat"),
              QStringLiteral("General"), 0, QString(), QString(), QString::number(caps));
}

QString writeTempFile(QTemporaryDir& dir, const QString& name) {
  QString path = dir.path() + QLatin1Char('/') + name;
  QFile f(path);
  if(!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return QString();
  f.write("dnd");
  f.close();
  return path;
}

struct Sent {
  QVector<int> types;
  QStringList paths;
};

void dropOnInput(lmcChatWindow& window, const QMimeData& mime, bool* enterAccepted = nullptr,
                 bool* dropAccepted = nullptr) {
  QTextEdit* input = window.findChild<QTextEdit*>(QStringLiteral("txtMessage"));
  QVERIFY(input != nullptr);
  QWidget* viewport = input->viewport();
  QVERIFY(viewport != nullptr);
  QDragEnterEvent enter(QPoint(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
  QCoreApplication::sendEvent(viewport, &enter);
  if(enterAccepted)
    *enterAccepted = enter.isAccepted();
  QDropEvent drop(QPointF(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
  QCoreApplication::sendEvent(viewport, &drop);
  if(dropAccepted)
    *dropAccepted = drop.isAccepted();
}

} // namespace

class TstChatDnd : public QObject {
  Q_OBJECT

private slots:
  void fileDropSendsFile();
  void folderDropSendsFolder();
  void folderWithoutFolderCapSwallowed();
  void fileWithoutCapsSwallowed();
  void missingPathSwallowed();
  void httpUrlInsertsText();
  void mixedFolderFirstSendsFileOnly();
};

void TstChatDnd::fileDropSendsFile() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QString filePath = writeTempFile(dir, QStringLiteral("drop-file.txt"));
  QVERIFY(!filePath.isEmpty());

  User local = makeUser(QStringLiteral("me"), QStringLiteral("Me"), UC_File | UC_Folder);
  User remote = makeUser(QStringLiteral("dndpeer"), QStringLiteral("Peer"), UC_File | UC_Folder);
  lmcChatWindow window;
  window.init(&local, &remote, true);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));
  QTextEdit* input = window.findChild<QTextEdit*>(QStringLiteral("txtMessage"));
  QVERIFY(input != nullptr);

  Sent sent;
  connect(&window, &lmcChatWindow::messageSent, &window,
          [&](MessageType t, QString*, XmlMessage* m) {
            sent.types.append(static_cast<int>(t));
            sent.paths.append(m ? m->data(XN_FILEPATH) : QString());
          });

  QMimeData mime;
  mime.setUrls({QUrl::fromLocalFile(filePath)});
  bool enterAccepted = false;
  dropOnInput(window, mime, &enterAccepted);
  QVERIFY(enterAccepted);
  QCOMPARE(sent.types.size(), 1);
  QCOMPARE(sent.types.at(0), static_cast<int>(MT_File));
  QCOMPARE(sent.paths.at(0), filePath);
  QVERIFY(input->toPlainText().isEmpty());
}

void TstChatDnd::folderDropSendsFolder() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  User local = makeUser(QStringLiteral("me"), QStringLiteral("Me"), UC_File | UC_Folder);
  User remote = makeUser(QStringLiteral("dndpeer"), QStringLiteral("Peer"), UC_File | UC_Folder);
  lmcChatWindow window;
  window.init(&local, &remote, true);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

  Sent sent;
  connect(&window, &lmcChatWindow::messageSent, &window,
          [&](MessageType t, QString*, XmlMessage* m) {
            sent.types.append(static_cast<int>(t));
            sent.paths.append(m ? m->data(XN_FILEPATH) : QString());
          });

  QMimeData mime;
  mime.setUrls({QUrl::fromLocalFile(dir.path())});
  dropOnInput(window, mime);
  QCOMPARE(sent.types.size(), 1);
  QCOMPARE(sent.types.at(0), static_cast<int>(MT_Folder));
  QCOMPARE(sent.paths.at(0), dir.path());
}

void TstChatDnd::folderWithoutFolderCapSwallowed() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  User local = makeUser(QStringLiteral("me"), QStringLiteral("Me"), UC_File | UC_Folder);
  User remote = makeUser(QStringLiteral("dndpeer"), QStringLiteral("Peer"), UC_File);
  lmcChatWindow window;
  window.init(&local, &remote, true);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));
  QTextEdit* input = window.findChild<QTextEdit*>(QStringLiteral("txtMessage"));
  QVERIFY(input != nullptr);

  Sent sent;
  connect(&window, &lmcChatWindow::messageSent, &window,
          [&](MessageType t, QString*, XmlMessage* m) {
            sent.types.append(static_cast<int>(t));
            sent.paths.append(m ? m->data(XN_FILEPATH) : QString());
          });

  QMimeData mime;
  mime.setUrls({QUrl::fromLocalFile(dir.path())});
  dropOnInput(window, mime);
  QCOMPARE(sent.types.size(), 0);
  QVERIFY(input->toPlainText().isEmpty());
}

void TstChatDnd::fileWithoutCapsSwallowed() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QString filePath = writeTempFile(dir, QStringLiteral("drop-file.txt"));
  QVERIFY(!filePath.isEmpty());

  User local = makeUser(QStringLiteral("me"), QStringLiteral("Me"), UC_File | UC_Folder);
  User remote = makeUser(QStringLiteral("dndpeer"), QStringLiteral("Peer"), UC_None);
  lmcChatWindow window;
  window.init(&local, &remote, true);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));
  QTextEdit* input = window.findChild<QTextEdit*>(QStringLiteral("txtMessage"));
  QVERIFY(input != nullptr);

  Sent sent;
  connect(&window, &lmcChatWindow::messageSent, &window,
          [&](MessageType t, QString*, XmlMessage* m) {
            sent.types.append(static_cast<int>(t));
            sent.paths.append(m ? m->data(XN_FILEPATH) : QString());
          });

  QMimeData mime;
  mime.setUrls({QUrl::fromLocalFile(filePath)});
  dropOnInput(window, mime);
  QCOMPARE(sent.types.size(), 0);
  QVERIFY(input->toPlainText().isEmpty());
}

void TstChatDnd::missingPathSwallowed() {
  User local = makeUser(QStringLiteral("me"), QStringLiteral("Me"), UC_File | UC_Folder);
  User remote = makeUser(QStringLiteral("dndpeer"), QStringLiteral("Peer"), UC_File | UC_Folder);
  lmcChatWindow window;
  window.init(&local, &remote, true);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));
  QTextEdit* input = window.findChild<QTextEdit*>(QStringLiteral("txtMessage"));
  QVERIFY(input != nullptr);

  Sent sent;
  connect(&window, &lmcChatWindow::messageSent, &window,
          [&](MessageType t, QString*, XmlMessage* m) {
            sent.types.append(static_cast<int>(t));
            sent.paths.append(m ? m->data(XN_FILEPATH) : QString());
          });

  QString missing = QDir::tempPath() + QStringLiteral("/lmc-dnd-missing.txt");
  QVERIFY(!QFileInfo::exists(missing));
  QMimeData mime;
  mime.setUrls({QUrl::fromLocalFile(missing)});
  dropOnInput(window, mime);
  QCOMPARE(sent.types.size(), 0);
  QVERIFY(input->toPlainText().isEmpty());
}

void TstChatDnd::httpUrlInsertsText() {
  User local = makeUser(QStringLiteral("me"), QStringLiteral("Me"), UC_File | UC_Folder);
  User remote = makeUser(QStringLiteral("dndpeer"), QStringLiteral("Peer"), UC_File | UC_Folder);
  lmcChatWindow window;
  window.init(&local, &remote, true);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));
  QTextEdit* input = window.findChild<QTextEdit*>(QStringLiteral("txtMessage"));
  QVERIFY(input != nullptr);

  Sent sent;
  connect(&window, &lmcChatWindow::messageSent, &window,
          [&](MessageType t, QString*, XmlMessage* m) {
            sent.types.append(static_cast<int>(t));
            sent.paths.append(m ? m->data(XN_FILEPATH) : QString());
          });

  QMimeData mime;
  mime.setUrls({QUrl(QStringLiteral("https://example.com/"))});
  dropOnInput(window, mime);
  QCOMPARE(sent.types.size(), 0);
  QVERIFY(input->toPlainText().contains(QStringLiteral("example.com")));
}

void TstChatDnd::mixedFolderFirstSendsFileOnly() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QString subDirPath = dir.path() + QStringLiteral("/sub");
  QVERIFY(QDir().mkpath(subDirPath));
  QString filePath = writeTempFile(dir, QStringLiteral("drop-file.txt"));
  QVERIFY(!filePath.isEmpty());

  User local = makeUser(QStringLiteral("me"), QStringLiteral("Me"), UC_File | UC_Folder);
  User remote = makeUser(QStringLiteral("dndpeer"), QStringLiteral("Peer"), UC_File);
  lmcChatWindow window;
  window.init(&local, &remote, true);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window, 5000));

  Sent sent;
  connect(&window, &lmcChatWindow::messageSent, &window,
          [&](MessageType t, QString*, XmlMessage* m) {
            sent.types.append(static_cast<int>(t));
            sent.paths.append(m ? m->data(XN_FILEPATH) : QString());
          });

  QMimeData mime;
  mime.setUrls({QUrl::fromLocalFile(subDirPath), QUrl::fromLocalFile(filePath)});
  dropOnInput(window, mime);
  QCOMPARE(sent.types.size(), 1);
  QCOMPARE(sent.types.at(0), static_cast<int>(MT_File));
  QCOMPARE(sent.paths.at(0), filePath);
}

QTEST_MAIN(TstChatDnd)
#include "tst_chatdnd.moc"
