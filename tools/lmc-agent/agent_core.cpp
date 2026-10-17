#include "agent_core.h"

// Item data roles mirror lmc/src/core/uidefinitions.h:84-92
// (IdRole = Qt::UserRole + 1, TypeRole next). Kept as literals on purpose:
// the agent links no lmc headers, so it cannot include the enum, and a
// wrong literal here fails loudly (peer never resolves) instead of silently.

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonArray>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPointer>
#include <QRegularExpression>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>

namespace lmc_agent {
namespace {

constexpr int kIdRole = Qt::UserRole + 1;
constexpr int kTypeRole = Qt::UserRole + 2;

QJsonObject normalizedCriteria(const QJsonObject& raw) {
  // qtexpert-compatible aliases (see locators.py in robotframework-qtexpert).
  QJsonObject out;
  for(auto it = raw.begin(); it != raw.end(); ++it) {
    const QString key = it.key().toLower();
    QString norm = it.key();
    if(key == "name" || key == "objectname")
      norm = "objectName";
    else if(key == "type" || key == "class" || key == "classname")
      norm = "className";
    else if(key == "text" || key == "title")
      norm = "text";
    else if(key == "window" || key == "windowtitle")
      norm = "windowTitle";
    else if(key == "visible" || key == "enabled")
      norm = key;
    out[norm] = it.value();
  }
  return out;
}

bool toBool(const QJsonValue& value, bool* ok) {
  if(value.isBool()) {
    *ok = true;
    return value.toBool();
  }
  if(value.isString()) {
    const QString s = value.toString().toLower();
    if(s == "true" || s == "1") {
      *ok = true;
      return true;
    }
    if(s == "false" || s == "0") {
      *ok = true;
      return false;
    }
  }
  *ok = false;
  return false;
}

QString strippedActionText(const QString& text) {
  QString out = text;
  out.remove(QLatin1Char('&'));
  return out;
}

bool widgetMatches(QWidget* widget, const QJsonObject& criteria) {
  for(auto it = criteria.begin(); it != criteria.end(); ++it) {
    const QString key = it.key();
    const QJsonValue want = it.value();
    if(key == "objectName") {
      if(widget->objectName() != want.toString())
        return false;
    } else if(key == "className") {
      const QString name = want.toString();
      if(QLatin1String(widget->metaObject()->className()) != name &&
         !widget->inherits(name.toLatin1().constData()))
        return false;
    } else if(key == "text") {
      QString have;
      const QVariant prop = widget->property("text");
      if(prop.isValid())
        have = prop.toString();
      else
        have = widget->windowTitle();
      if(strippedActionText(have) != strippedActionText(want.toString()))
        return false;
    } else if(key == "windowTitle") {
      if(widget->window()->windowTitle() != want.toString())
        return false;
    } else if(key == "visible") {
      bool ok = false;
      const bool expected = toBool(want, &ok);
      if(!ok || widget->isVisible() != expected)
        return false;
    } else if(key == "enabled") {
      bool ok = false;
      const bool expected = toBool(want, &ok);
      if(!ok || widget->isEnabled() != expected)
        return false;
    } else {
      return false; // unknown key: no silent partial match
    }
  }
  return true;
}

bool actionMatches(QAction* action, const QJsonObject& criteria) {
  for(auto it = criteria.begin(); it != criteria.end(); ++it) {
    const QString key = it.key();
    const QJsonValue want = it.value();
    if(key == "text") {
      if(strippedActionText(action->text()) !=
         strippedActionText(want.toString()))
        return false;
    } else if(key == "objectName" || key == "name") {
      if(action->objectName() != want.toString())
        return false;
    } else if(key == "enabled") {
      bool ok = false;
      const bool expected = toBool(want, &ok);
      if(!ok || action->isEnabled() != expected)
        return false;
    } else if(key == "visible") {
      bool ok = false;
      const bool expected = toBool(want, &ok);
      if(!ok || action->isVisible() != expected)
        return false;
    } else {
      return false;
    }
  }
  return true;
}

void collectActions(QObject* holder, QList<QAction*>* out) {
  QList<QAction*> direct;
  if(auto* bar = qobject_cast<QMenuBar*>(holder))
    direct = bar->actions();
  else if(auto* tool = qobject_cast<QToolBar*>(holder))
    direct = tool->actions();
  else if(auto* menu = qobject_cast<QMenu*>(holder))
    direct = menu->actions();
  else if(auto* button = qobject_cast<QToolButton*>(holder)) {
    direct = button->actions();
    if(button->menu())
      direct += button->menu()->actions();
  }
  for(QAction* action : direct) {
    if(!out->contains(action))
      out->append(action);
    if(action->menu()) {
      // Recurse into submenus.
      QList<QAction*> sub;
      sub.append(action->menu()->actions());
      for(QAction* inner : sub) {
        if(!out->contains(inner))
          out->append(inner);
        if(inner->menu())
          collectActions(inner->menu(), out);
      }
    }
  }
}

QList<QAction*> allActions() {
  QList<QAction*> out;
  const QWidgetList tops = QApplication::topLevelWidgets();
  for(QWidget* top : tops) {
    collectActions(top, &out);
    const QList<QMenuBar*> bars =
      top->findChildren<QMenuBar*>(QString(), Qt::FindChildrenRecursively);
    const QList<QToolBar*> tools =
      top->findChildren<QToolBar*>(QString(), Qt::FindChildrenRecursively);
    const QList<QMenu*> menus =
      top->findChildren<QMenu*>(QString(), Qt::FindChildrenRecursively);
    const QList<QToolButton*> buttons = top->findChildren<QToolButton*>(
      QString(), Qt::FindChildrenRecursively);
    for(QMenuBar* bar : bars)
      collectActions(bar, &out);
    for(QToolBar* tool : tools)
      collectActions(tool, &out);
    for(QMenu* menu : menus)
      collectActions(menu, &out);
    for(QToolButton* button : buttons)
      collectActions(button, &out);
  }
  return out;
}

QWidget* mainWindow() {
  const QWidgetList tops = QApplication::topLevelWidgets();
  for(QWidget* top : tops) {
    if(top->objectName() == QStringLiteral("MainWindow"))
      return top;
  }
  return nullptr;
}

QSet<QWidget*> visibleChatWindows() {
  QSet<QWidget*> out;
  const QWidgetList tops = QApplication::topLevelWidgets();
  for(QWidget* top : tops) {
    if(top->objectName() == QStringLiteral("ChatWindow") && top->isVisible())
      out.insert(top);
  }
  return out;
}

QJsonObject describeAction(QAction* action) {
  QJsonObject node;
  node[QStringLiteral("kind")] = QStringLiteral("action");
  node[QStringLiteral("objectName")] = action->objectName();
  node[QStringLiteral("text")] = strippedActionText(action->text());
  node[QStringLiteral("enabled")] = action->isEnabled();
  node[QStringLiteral("visible")] = action->isVisible();
  return node;
}

QJsonObject dumpNode(QWidget* widget) {
  QJsonObject node = describeWidget(widget);
  QJsonArray kids;
  const QObjectList direct = widget->children();
  for(QObject* child : direct) {
    if(auto* childWidget = qobject_cast<QWidget*>(child))
      kids.append(dumpNode(childWidget));
  }
  if(!kids.isEmpty())
    node[QStringLiteral("children")] = kids;
  QList<QAction*> actions;
  collectActions(widget, &actions);
  // Only report actions owned directly (not the whole subtree twice).
  QJsonArray actionNodes;
  for(QAction* action : actions) {
    auto* owner = qobject_cast<QWidget*>(action->parent());
    if(action->parent() == widget || (owner && widget->isAncestorOf(owner)))
      actionNodes.append(describeAction(action));
  }
  if(!actionNodes.isEmpty())
    node[QStringLiteral("actions")] = actionNodes;
  return node;
}

} // namespace

QJsonObject parseLocator(const QString& locator) {
  const QString trimmed = locator.trimmed();
  static const QRegularExpression pairRe(
    QStringLiteral("(\\w+)=(?:\"([^\"]*)\"|'([^']*)'|(\\S+))"));
  QJsonObject raw;
  auto it = pairRe.globalMatch(trimmed);
  bool any = false;
  while(it.hasNext()) {
    const auto match = it.next();
    any = true;
    QString value = match.captured(2);
    if(value.isNull())
      value = match.captured(3);
    if(value.isNull())
      value = match.captured(4);
    const QString key = match.captured(1).toLower();
    if(value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0)
      raw[match.captured(1)] = true;
    else if(value.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0)
      raw[match.captured(1)] = false;
    else
      raw[match.captured(1)] = value;
    Q_UNUSED(key);
  }
  if(!any)
    raw[QStringLiteral("name")] = trimmed;
  return normalizedCriteria(raw);
}

QWidget* findWidget(const QJsonObject& criteria) {
  const QWidgetList tops = QApplication::topLevelWidgets();
  for(QWidget* top : tops) {
    if(widgetMatches(top, criteria))
      return top;
    const QList<QWidget*> kids = top->findChildren<QWidget*>(
      QString(), Qt::FindChildrenRecursively);
    for(QWidget* kid : kids) {
      if(widgetMatches(kid, criteria))
        return kid;
    }
  }
  return nullptr;
}

QAction* findAction(const QJsonObject& criteria) {
  const QList<QAction*> actions = allActions();
  for(QAction* action : actions) {
    if(actionMatches(action, criteria))
      return action;
  }
  return nullptr;
}

QJsonObject describeWidget(QWidget* widget) {
  QJsonObject node;
  node[QStringLiteral("kind")] = QStringLiteral("widget");
  node[QStringLiteral("objectName")] = widget->objectName();
  node[QStringLiteral("class")] = QString::fromLatin1(widget->metaObject()->className());
  node[QStringLiteral("visible")] = widget->isVisible();
  node[QStringLiteral("enabled")] = widget->isEnabled();
  if(!widget->windowTitle().isEmpty())
    node[QStringLiteral("title")] = widget->windowTitle();
  const QVariant textProp = widget->property("text");
  if(textProp.isValid() && textProp.canConvert<QString>()) {
    const QString text = textProp.toString();
    if(!text.isEmpty())
      node[QStringLiteral("text")] = text;
  }
  return node;
}

QJsonArray dumpTree() {
  QJsonArray out;
  const QWidgetList tops = QApplication::topLevelWidgets();
  for(QWidget* top : tops)
    out.append(dumpNode(top));
  return out;
}

QJsonArray listWindows() {
  QJsonArray out;
  const QWidgetList tops = QApplication::topLevelWidgets();
  for(QWidget* top : tops) {
    QJsonObject entry;
    entry[QStringLiteral("objectName")] = top->objectName();
    entry[QStringLiteral("class")] =
      QString::fromLatin1(top->metaObject()->className());
    entry[QStringLiteral("title")] = top->windowTitle();
    entry[QStringLiteral("visible")] = top->isVisible();
    out.append(entry);
  }
  return out;
}

bool postClick(const QJsonObject& criteria, QString* error) {
  if(QWidget* widget = findWidget(criteria)) {
    const QPointF center(widget->rect().center());
    auto* press = new QMouseEvent(QEvent::MouseButtonPress, center,
                                  widget->mapToGlobal(center.toPoint()),
                                  Qt::LeftButton, Qt::LeftButton,
                                  Qt::NoModifier);
    auto* release = new QMouseEvent(QEvent::MouseButtonRelease, center,
                                    widget->mapToGlobal(center.toPoint()),
                                    Qt::LeftButton, Qt::NoButton,
                                    Qt::NoModifier);
    // Fire-and-post: reply is sent by the caller immediately, the events
    // land on the next loop iterations. Never blocks on exec() modals.
    QCoreApplication::postEvent(widget, press);
    QCoreApplication::postEvent(widget, release);
    return true;
  }
  if(QAction* action = findAction(criteria)) {
    QPointer<QAction> guard(action);
    QTimer::singleShot(0, [guard]() {
      if(guard)
        guard->trigger();
    });
    return true;
  }
  if(error)
    *error = QStringLiteral("no widget or action matches locator");
  return false;
}

bool triggerAction(const QJsonObject& criteria, QString* triggeredText,
                   QString* error) {
  // Two passes: leaf commands first, submenu owners second. A menu title
  // often repeats one of its items (Help menu vs Help item): the bare
  // menuAction only pops the dropdown, while the item runs the slot.
  // allActions() order follows topLevelWidgets(), which is not stable
  // (a popup-flagged QMenu is visited both as a top and as a child), so
  // first-match-wins would be a coin flip.
  const QList<QAction*> actions = allActions();
  QAction* action = nullptr;
  for(QAction* candidate : actions) {
    if(candidate->menu() == nullptr && actionMatches(candidate, criteria)) {
      action = candidate;
      break;
    }
  }
  if(!action) {
    for(QAction* candidate : actions) {
      if(actionMatches(candidate, criteria)) {
        action = candidate;
        break;
      }
    }
  }
  if(!action) {
    if(error)
      *error = QStringLiteral("no action matches locator");
    return false;
  }
  // Same fire-and-post shape as the QAction branch of postClick: the
  // reply goes out before the slot runs, so an exec() modal opened by
  // the trigger cannot swallow the connection.
  QPointer<QAction> guard(action);
  QTimer::singleShot(0, [guard]() {
    if(guard)
      guard->trigger();
  });
  if(triggeredText)
    *triggeredText = strippedActionText(action->text());
  return true;
}

bool closeWindow(const QJsonObject& criteria, QJsonObject* closed,
                 QString* error) {
  QWidget* match = findWidget(criteria);
  if(!match) {
    if(error)
      *error = QStringLiteral("no widget matches locator");
    return false;
  }
  // Resolve to the top-level: closing a child would be a no-op.
  QWidget* window = match->isWindow() ? match : match->window();
  // Fire-and-post, same reason as postClick/triggerAction: the reply must
  // precede whatever closeEvent handlers do (lmcChatWindow saves history
  // there; reused modeless windows only hide).
  QPointer<QWidget> guard(window);
  QTimer::singleShot(0, [guard]() {
    if(guard)
      guard->close();
  });
  if(closed) {
    (*closed)[QStringLiteral("objectName")] = window->objectName();
    (*closed)[QStringLiteral("class")] =
      QString::fromLatin1(window->metaObject()->className());
    (*closed)[QStringLiteral("title")] = window->windowTitle();
    // Snapshot at post time (the close itself is async, see above):
    // visible tells whether there was anything to hide, isWindow whether
    // the locator hit the top level directly, ptr lets a later
    // wait_for_gone verdict be correlated to this exact object.
    (*closed)[QStringLiteral("visible")] = window->isVisible();
    (*closed)[QStringLiteral("isWindow")] = match->isWindow();
    (*closed)[QStringLiteral("ptr")] =
      QString::number(reinterpret_cast<quintptr>(window));
  }
  return true;
}

bool keyFromName(const QString& name, int* key, QString* error) {
  // Small table on purpose: only keys E2E drives (chat send, dialog
  // Tab-chains, Escape closes). No printable characters — text entry
  // goes through typeText, like xdotool type vs key.
  static const struct {
    const char* name;
    int key;
  } table[] = {
    {"return", Qt::Key_Return},
    {"enter", Qt::Key_Enter},
    {"tab", Qt::Key_Tab},
    {"backtab", Qt::Key_Backtab},
    {"escape", Qt::Key_Escape},
    {"esc", Qt::Key_Escape},
    {"space", Qt::Key_Space},
    {"backspace", Qt::Key_Backspace},
    {"delete", Qt::Key_Delete},
    {"home", Qt::Key_Home},
    {"end", Qt::Key_End},
    {"left", Qt::Key_Left},
    {"right", Qt::Key_Right},
    {"up", Qt::Key_Up},
    {"down", Qt::Key_Down},
  };
  const QString lower = name.toLower();
  for(const auto& entry : table) {
    if(lower == QString::fromLatin1(entry.name)) {
      *key = entry.key;
      return true;
    }
  }
  if(error)
    *error = QStringLiteral("unknown key name: %1").arg(name);
  return false;
}

Qt::KeyboardModifiers modifiersFromNames(const QJsonArray& names) {
  Qt::KeyboardModifiers mods = Qt::NoModifier;
  for(const QJsonValue& value : names) {
    const QString name = value.toString().toLower();
    if(name == QStringLiteral("ctrl") || name == QStringLiteral("control"))
      mods |= Qt::ControlModifier;
    else if(name == QStringLiteral("shift"))
      mods |= Qt::ShiftModifier;
    else if(name == QStringLiteral("alt"))
      mods |= Qt::AltModifier;
    else if(name == QStringLiteral("meta"))
      mods |= Qt::MetaModifier;
  }
  return mods;
}

bool postKey(QWidget* target, const QString& keyName,
             Qt::KeyboardModifiers mods, QString* error) {
  int key = 0;
  if(!keyFromName(keyName, &key, error))
    return false;
  QObject* receiver = target;
  if(!receiver)
    receiver = QApplication::focusWidget();
  if(!receiver)
    receiver = QApplication::activeWindow();
  if(!receiver) {
    if(error)
      *error = QStringLiteral("no target widget (no focus, no active window)");
    return false;
  }
  // Same fire-and-post shape as postClick: posted press+release land on
  // later loop iterations, after the reply. installEventFilter hooks
  // (ChatWindow::eventFilter for txtMessage) see posted events too.
  auto* press = new QKeyEvent(QEvent::KeyPress, key, mods);
  auto* release = new QKeyEvent(QEvent::KeyRelease, key, mods);
  QCoreApplication::postEvent(receiver, press);
  QCoreApplication::postEvent(receiver, release);
  return true;
}

bool typeText(const QJsonObject& criteria, const QString& text, bool send,
              QString* error) {
  QWidget* widget = findWidget(criteria);
  if(!widget) {
    if(error)
      *error = QStringLiteral("no widget matches locator");
    return false;
  }
  if(auto* edit = qobject_cast<QTextEdit*>(widget))
    edit->setPlainText(text);
  else if(auto* plain = qobject_cast<QPlainTextEdit*>(widget))
    plain->setPlainText(text);
  else if(auto* line = qobject_cast<QLineEdit*>(widget))
    line->setText(text);
  else {
    if(error)
      *error = QStringLiteral("not an editable text widget: %1").arg(QString::fromLatin1(widget->metaObject()->className()));
    return false;
  }
  // Direct set (never blocks: no exec() involved). The Return press is
  // posted, so the widget's own Enter handling runs exactly as with a
  // real key — including the SendKeyMod setting check.
  if(send && !postKey(widget, QStringLiteral("Return"), Qt::NoModifier, error))
    return false;
  return true;
}

bool waitForObject(const QJsonObject& criteria, double timeoutSecs,
                   QJsonObject* found, QString* error) {
  if(QWidget* widget = findWidget(criteria)) {
    if(found)
      *found = describeWidget(widget);
    return true;
  }
  if(QAction* action = findAction(criteria)) {
    if(found)
      *found = describeAction(action);
    return true;
  }
  QEventLoop loop;
  bool hit = false;
  QJsonObject seen;
  QTimer poll;
  poll.setInterval(100);
  QObject::connect(&poll, &QTimer::timeout, [&]() {
    if(QWidget* widget = findWidget(criteria)) {
      seen = describeWidget(widget);
      hit = true;
      loop.quit();
    } else if(QAction* action = findAction(criteria)) {
      seen = describeAction(action);
      hit = true;
      loop.quit();
    }
  });
  QTimer::singleShot(int(timeoutSecs * 1000.0), &loop, &QEventLoop::quit);
  poll.start();
  loop.exec();
  if(hit) {
    if(found)
      *found = seen;
    return true;
  }
  if(error)
    *error = QStringLiteral("not found within %1s").arg(timeoutSecs);
  return false;
}

bool waitForGone(const QJsonObject& criteria, double timeoutSecs,
                 QString* error) {
  if(!findWidget(criteria) && !findAction(criteria))
    return true;
  // Identity at entry: distinguishes "close never ran" (the same object is
  // still there) from "recreated" (a new object matches now).
  QWidget* first = findWidget(criteria);
  const quintptr firstPtr = first ? reinterpret_cast<quintptr>(first) : 0;
  QEventLoop loop;
  bool gone = false;
  QTimer poll;
  poll.setInterval(100);
  QObject::connect(&poll, &QTimer::timeout, [&]() {
    if(!findWidget(criteria) && !findAction(criteria)) {
      gone = true;
      loop.quit();
    }
  });
  QTimer::singleShot(int(timeoutSecs * 1000.0), &loop, &QEventLoop::quit);
  poll.start();
  loop.exec();
  if(gone)
    return true;
  if(!error)
    return false;
  // Recheck after the timeout: the window may have vanished in between.
  if(QWidget* stuck = findWidget(criteria)) {
    const bool same = reinterpret_cast<quintptr>(stuck) == firstPtr;
    *error = QStringLiteral("still present after %1s (class=%2 title=\"%3\" visible=%4 sameWindow=%5)")
               .arg(timeoutSecs)
               .arg(QString::fromLatin1(stuck->metaObject()->className()))
               .arg(stuck->windowTitle())
               .arg(stuck->isVisible() ? QStringLiteral("true") : QStringLiteral("false"))
               .arg(same ? QStringLiteral("true") : QStringLiteral("false"));
    return false;
  }
  if(QAction* action = findAction(criteria)) {
    *error = QStringLiteral("still present after %1s (action text=\"%2\")")
               .arg(timeoutSecs)
               .arg(strippedActionText(action->text()));
    return false;
  }
  // Vanished between the timeout firing and this recheck: the contract holds.
  return true;
}

bool openChat(const QString& peer, double timeoutSecs, QString* title,
              QString* error) {
  QWidget* main = mainWindow();
  if(!main) {
    if(error)
      *error = QStringLiteral("MainWindow not found");
    return false;
  }
  auto* tree = main->findChild<QTreeWidget*>(QStringLiteral("tvUserList"));
  if(!tree) {
    if(error)
      *error = QStringLiteral("tvUserList not found under MainWindow");
    return false;
  }

  // Two passes: userId first (exact identity), display name second —
  // same precedence as the app's own lookups.
  QTreeWidgetItem* match = nullptr;
  QString matchName;
  const auto collect = [&](bool byId) -> QTreeWidgetItem* {
    QTreeWidgetItem* fallback = nullptr;
    std::function<void(QTreeWidgetItem*)> walk = [&](QTreeWidgetItem* item) {
      if(!item || fallback)
        return;
      if(item->data(0, kTypeRole).toString() == QStringLiteral("User")) {
        const QString id = item->data(0, kIdRole).toString();
        const QString name = item->text(0);
        const bool hit = byId ? (id == peer)
                              : (name == peer ||
                                 item->data(0, Qt::DisplayRole).toString() == peer);
        if(hit && !fallback) {
          fallback = item;
          return;
        }
      }
      for(int i = 0; i < item->childCount() && !fallback; ++i)
        walk(item->child(i));
    };
    for(int i = 0; i < tree->topLevelItemCount() && !fallback; ++i)
      walk(tree->topLevelItem(i));
    return fallback;
  };
  match = collect(true);
  if(!match)
    match = collect(false);
  if(!match) {
    if(error)
      *error = QStringLiteral("peer not found in contact tree: %1").arg(peer);
    return false;
  }
  matchName = match->text(0);

  const QSet<QWidget*> before = visibleChatWindows();

  // Primary: the same slot keyboard Return goes through
  // (tvUserList_itemActivated -> chatStarting -> lmcCore::startChat,
  // idempotent). Synchronous DirectConnection: startChat only creates and
  // shows a modeless window, so no nested-loop hazard. If the private
  // slot cannot be invoked by name (metatype gap), fall back to posting
  // Return to the tree — still in-process, no X focus needed.
  const bool invoked = QMetaObject::invokeMethod(
    main, "tvUserList_itemActivated", Qt::DirectConnection,
    Q_ARG(QTreeWidgetItem*, match), Q_ARG(int, 0));
  if(!invoked) {
    tree->setCurrentItem(match);
    tree->scrollToItem(match);
    auto* press = new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    auto* release =
      new QKeyEvent(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::postEvent(tree, press);
    QCoreApplication::postEvent(tree, release);
  }

  // Wait for the (possibly just shown) ChatWindow. The visible flag is set
  // synchronously by show(); the loop only bridges posted-event latency.
  QEventLoop loop;
  bool ok = false;
  QString seenTitle;
  QTimer poll;
  poll.setInterval(100);
  QObject::connect(&poll, &QTimer::timeout, [&]() {
    const QSet<QWidget*> after = visibleChatWindows();
    for(QWidget* w : after) {
      if(!before.contains(w)) {
        seenTitle = w->windowTitle();
        ok = true;
        loop.quit();
        return;
      }
    }
    // Idempotent reopen (window already visible): match by peer name.
    for(QWidget* w : after) {
      if(w->windowTitle().contains(matchName)) {
        seenTitle = w->windowTitle();
        ok = true;
        loop.quit();
        return;
      }
    }
  });
  QTimer::singleShot(int(timeoutSecs * 1000.0), &loop, &QEventLoop::quit);
  poll.start();
  loop.exec();
  if(ok) {
    if(title)
      *title = seenTitle;
    return true;
  }
  if(error)
    *error = QStringLiteral("chat window did not appear within %1s for peer %2")
               .arg(timeoutSecs)
               .arg(peer);
  return false;
}

} // namespace lmc_agent
