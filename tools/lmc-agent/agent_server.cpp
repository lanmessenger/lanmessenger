#include "agent_server.h"

#include <QApplication>
#include <QCoreApplication>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPointer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QTimer>
#include <QVariant>

#include "agent_core.h"

namespace lmc_agent {
namespace {

constexpr double kDefaultTimeoutSecs = 10.0;

double timeoutParam(const QJsonObject& params) {
  const QJsonValue value = params.value(QStringLiteral("timeout"));
  if(value.isDouble())
    return value.toDouble();
  if(value.isString()) {
    QString text = value.toString().trimmed().toLower();
    if(text.endsWith(QLatin1Char('s')))
      text.chop(1);
    bool ok = false;
    const double secs = text.toDouble(&ok);
    if(ok && secs > 0.0)
      return secs;
  }
  return kDefaultTimeoutSecs;
}

QJsonObject errorReply(const QJsonValue& id, const QString& error) {
  QJsonObject reply;
  reply[QStringLiteral("id")] = id;
  reply[QStringLiteral("ok")] = false;
  reply[QStringLiteral("error")] = error;
  return reply;
}

QJsonObject okReply(const QJsonValue& id, const QJsonObject& result) {
  QJsonObject reply;
  reply[QStringLiteral("id")] = id;
  reply[QStringLiteral("ok")] = true;
  reply[QStringLiteral("result")] = result;
  return reply;
}

// Per-connection state: buffers one NDJSON stream, dispatches complete
// lines in the GUI thread (all socket signals already live there).
class Connection : public QObject {
  Q_OBJECT

public:
  Connection(QTcpSocket* socket, AgentServer* server)
      : QObject(server), socket_(socket), server_(server) {
    socket_->setParent(this);
    connect(socket_, &QTcpSocket::readyRead, this, &Connection::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &Connection::deleteLater);
  }

private slots:
  void onReadyRead() {
    buffer_ += socket_->readAll();
    int newline = -1;
    while((newline = buffer_.indexOf('\n')) >= 0) {
      const QByteArray line = buffer_.left(newline).trimmed();
      buffer_.remove(0, newline + 1);
      if(line.isEmpty())
        continue;
      handleLine(line);
    }
  }

private:
  void send(const QJsonObject& reply) {
    socket_->write(QJsonDocument(reply).toJson(QJsonDocument::Compact) + "\n");
  }

  void handleLine(const QByteArray& line) {
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
    if(parseError.error != QJsonParseError::NoError || !doc.isObject()) {
      send(errorReply(0, QStringLiteral("invalid json")));
      return;
    }
    const QJsonObject request = doc.object();
    const QJsonValue id = request.value(QStringLiteral("id"));
    const QString cmd = request.value(QStringLiteral("cmd")).toString();
    const QJsonObject params = request.value(QStringLiteral("params")).toObject();
    if(cmd.isEmpty()) {
      send(errorReply(id, QStringLiteral("missing cmd")));
      return;
    }
    if(server_->isBusy()) {
      send(errorReply(id, QStringLiteral("busy")));
      return;
    }
    server_->setBusy(true);
    const QJsonObject reply = dispatch(id, cmd, params);
    server_->setBusy(false);
    send(reply);
    if(cmd == QStringLiteral("close_app")) {
      // Quit after the reply flushes: async so exec() modals (if any)
      // cannot swallow the response.
      QTimer::singleShot(0, qApp, &QCoreApplication::quit);
    }
  }

  QJsonObject dispatch(const QJsonValue& id, const QString& cmd,
                       const QJsonObject& params) {
    const double timeout = timeoutParam(params);
    if(cmd == QStringLiteral("wait_for_object")) {
      QJsonObject found;
      QString error;
      if(!waitForObject(parseLocator(params.value(QStringLiteral("locator")).toString()),
                        timeout, &found, &error))
        return errorReply(id, error);
      return okReply(id, found);
    }
    if(cmd == QStringLiteral("wait_for_gone")) {
      QString error;
      if(!waitForGone(parseLocator(params.value(QStringLiteral("locator")).toString()),
                      timeout, &error))
        return errorReply(id, error);
      QJsonObject result;
      result[QStringLiteral("gone")] = true;
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("object_exists")) {
      const QJsonObject criteria =
        parseLocator(params.value(QStringLiteral("locator")).toString());
      QJsonObject result;
      result[QStringLiteral("exists")] =
        (findWidget(criteria) != nullptr) || (findAction(criteria) != nullptr);
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("get_property")) {
      QWidget* widget =
        findWidget(parseLocator(params.value(QStringLiteral("locator")).toString()));
      if(!widget)
        return errorReply(id, QStringLiteral("no widget matches locator"));
      const QString name = params.value(QStringLiteral("property")).toString();
      QJsonObject result;
      // qtexpert parity: "text"/"plainText" on an editable widget means its
      // textual content — Qt exposes no such dynamic property on a
      // QTextEdit, only QLineEdit has "text".
      if(name == QStringLiteral("text") ||
         name == QStringLiteral("plainText")) {
        if(auto* textEdit = qobject_cast<QTextEdit*>(widget))
          result[QStringLiteral("value")] = textEdit->toPlainText();
        else if(auto* edit = qobject_cast<QPlainTextEdit*>(widget))
          result[QStringLiteral("value")] = edit->toPlainText();
        else if(auto* lineEdit = qobject_cast<QLineEdit*>(widget))
          result[QStringLiteral("value")] = lineEdit->text();
        else
          return errorReply(id,
                            QStringLiteral("%1 needs a text widget").arg(name));
      } else {
        const QVariant value = widget->property(name.toLatin1().constData());
        if(!value.isValid())
          return errorReply(id, QStringLiteral("unknown property: %1").arg(name));
        result[QStringLiteral("value")] = QJsonValue::fromVariant(value);
      }
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("set_property")) {
      QWidget* widget =
        findWidget(parseLocator(params.value(QStringLiteral("locator")).toString()));
      if(!widget)
        return errorReply(id, QStringLiteral("no widget matches locator"));
      const QString name = params.value(QStringLiteral("property")).toString();
      const QJsonValue value = params.value(QStringLiteral("value"));
      const bool set = widget->setProperty(name.toLatin1().constData(),
                                           value.toVariant());
      if(!set)
        return errorReply(id, QStringLiteral("cannot set property: %1").arg(name));
      QJsonObject result;
      result[QStringLiteral("set")] = true;
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("click")) {
      QString error;
      if(!postClick(parseLocator(params.value(QStringLiteral("locator")).toString()),
                    &error))
        return errorReply(id, error);
      QJsonObject result;
      result[QStringLiteral("clicked")] = true;
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("trigger_action")) {
      QString triggered;
      QString error;
      if(!triggerAction(parseLocator(params.value(QStringLiteral("locator")).toString()),
                        &triggered, &error))
        return errorReply(id, error);
      QJsonObject result;
      result[QStringLiteral("triggered")] = triggered;
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("type_text")) {
      const QString text = params.value(QStringLiteral("text")).toString();
      const QJsonValue sendValue = params.value(QStringLiteral("send"));
      const bool send = sendValue.isBool()
                          ? sendValue.toBool()
                          : sendValue.toString().compare(
                              QStringLiteral("true"),
                              Qt::CaseInsensitive) == 0;
      QString error;
      if(!typeText(parseLocator(params.value(QStringLiteral("locator")).toString()),
                   text, send, &error))
        return errorReply(id, error);
      QJsonObject result;
      result[QStringLiteral("typed")] = true;
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("send_key")) {
      QWidget* widget = nullptr;
      const QString locator = params.value(QStringLiteral("locator")).toString();
      if(!locator.isEmpty()) {
        widget = findWidget(parseLocator(locator));
        if(!widget)
          return errorReply(id, QStringLiteral("no widget matches locator"));
      }
      const QString key = params.value(QStringLiteral("key")).toString();
      if(key.isEmpty())
        return errorReply(id, QStringLiteral("missing key"));
      QString error;
      if(!postKey(widget, key,
                  modifiersFromNames(params.value(QStringLiteral("modifiers"))
                                       .toArray()),
                  &error))
        return errorReply(id, error);
      QJsonObject result;
      result[QStringLiteral("sent")] = key;
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("close_window")) {
      QJsonObject closed;
      QString error;
      if(!closeWindow(parseLocator(params.value(QStringLiteral("locator")).toString()),
                      &closed, &error))
        return errorReply(id, error);
      return okReply(id, closed);
    }
    if(cmd == QStringLiteral("dump_tree")) {
      QJsonObject result;
      result[QStringLiteral("tree")] = dumpTree();
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("get_log")) {
      QJsonObject result;
      result[QStringLiteral("path")] = server_->appLogPath();
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("list_windows")) {
      QJsonObject result;
      result[QStringLiteral("windows")] = listWindows();
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("grab_screenshot")) {
      QWidget* widget = nullptr;
      const QString locator = params.value(QStringLiteral("locator")).toString();
      if(!locator.isEmpty()) {
        widget = findWidget(parseLocator(locator));
        if(!widget)
          return errorReply(id, QStringLiteral("no widget matches locator"));
      } else {
        const QWidgetList tops = QApplication::topLevelWidgets();
        for(QWidget* top : tops) {
          if(top->objectName() == QStringLiteral("MainWindow")) {
            widget = top;
            break;
          }
        }
        if(!widget && !tops.isEmpty())
          widget = tops.first();
      }
      const QString file = params.value(QStringLiteral("file")).toString();
      if(file.isEmpty())
        return errorReply(id, QStringLiteral("missing file"));
      const QPixmap shot = widget->grab();
      if(shot.isNull() || !shot.save(file)) {
        return errorReply(id, QStringLiteral("screenshot failed: %1").arg(file));
      }
      QJsonObject result;
      result[QStringLiteral("file")] = file;
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("open_chat")) {
      QString title;
      QString error;
      if(!openChat(params.value(QStringLiteral("peer")).toString(), timeout,
                   &title, &error))
        return errorReply(id, error);
      QJsonObject result;
      result[QStringLiteral("title")] = title;
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("close_app")) {
      QJsonObject result;
      result[QStringLiteral("closing")] = true;
      return okReply(id, result);
    }
    if(cmd == QStringLiteral("open_window") || cmd == QStringLiteral("close_dialog") ||
       cmd == QStringLiteral("type_path")) {
      return errorReply(id, QStringLiteral("%1: not implemented (phase 2)").arg(cmd));
    }
    return errorReply(id, QStringLiteral("unknown cmd: %1").arg(cmd));
  }

  QTcpSocket* socket_;
  AgentServer* server_;
  QByteArray buffer_;
};

} // namespace

AgentServer::AgentServer(quint16 port, const QString& appLogPath,
                         QObject* parent)
    : QTcpServer(parent), appLogPath_(appLogPath) {
  if(!listen(QHostAddress::LocalHost, port)) {
    qWarning("lmc-agent: cannot listen on 127.0.0.1:%d: %s", port,
             qPrintable(errorString()));
    return;
  }
  connect(this, &QTcpServer::newConnection, this,
          &AgentServer::onNewConnection);
}

void AgentServer::onNewConnection() {
  while(hasPendingConnections()) {
    QTcpSocket* socket = nextPendingConnection();
    new Connection(socket, this);
  }
}

} // namespace lmc_agent

#include "agent_server.moc"
