#ifndef LMC_AGENT_CORE_H
#define LMC_AGENT_CORE_H

// In-process GUI adapter core: search + fire-and-post executors.
// Pure Qt Widgets API only — deliberately no lmc headers, so the same
// object file links both into the LD_PRELOAD .so (phase A) and into the
// lan-messenger-test binary (phase B) without ODR/duplication hazards.
//
// All entry points must run in the application's GUI thread (the
// QTcpServer lives there). Handlers may spin a bounded local QEventLoop
// while waiting for a window; see PROTOCOL.md ("busy" guard lives in
// the server layer).

#include <QAction>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QWidget>

namespace lmc_agent {

// Parse "name=X type=Y visible=true" into a criteria map.
// Bare string without '=' means {name: <string>}.
QJsonObject parseLocator(const QString& locator);

// First matching widget, or nullptr. Covers topLevelWidgets() +
// findChildren<QWidget*>() (QAction matches are reported via
// matchAction(), widgets only here).
::QWidget* findWidget(const QJsonObject& criteria);

// First matching QAction (menus/toolbars), or nullptr. qtexpert cannot
// see these at all; this is the parity+ gap.
::QAction* findAction(const QJsonObject& criteria);

// Describe one widget (shallow: no children).
QJsonObject describeWidget(::QWidget* widget);

// Full tree for dump_tree: top-level widgets, recursive children and
// actions as pseudo-nodes.
QJsonArray dumpTree();

// All top-level windows, flat.
QJsonArray listWindows();

// Fire-and-post click: QMouseEvent press+release posted to the widget
// (or async trigger() for a QAction). Returns immediately.
bool postClick(const QJsonObject& criteria, QString* error);

// Key/modifier name tables for send_key. Key names: Return, Enter, Tab,
// Backtab, Escape (Esc), Space, Backspace, Delete, Home, End, Left, Right,
// Up, Down (case-insensitive). Modifier names: ctrl/control, shift, alt,
// meta (case-insensitive, unknown names ignored).
bool keyFromName(const QString& name, int* key, QString* error);
Qt::KeyboardModifiers modifiersFromNames(const QJsonArray& names);

// Fire-and-post key press+release to a widget, or to the focus widget
// (else the active window) when target is null. Key names: Return, Enter,
// Tab, Backtab, Escape, Space, Backspace, Delete, Home, End, Left, Right,
// Up, Down (case-insensitive). Never blocks.
bool postKey(::QWidget* target, const QString& keyName,
             Qt::KeyboardModifiers mods, QString* error);

// Set editable text (QTextEdit/QPlainTextEdit/QLineEdit). With send=true
// a Return press is posted right after, so the widget's own Enter
// handling runs (e.g. ChatWindow::eventFilter -> sendMessage, honoring
// the SendKeyMod setting). Fire-and-post: returns immediately.
bool typeText(const QJsonObject& criteria, const QString& text, bool send,
              QString* error);

// Fire-and-post QAction trigger: finds a menu/toolbar action only
// (never a widget), queues trigger() via singleShot(0) so the reply
// goes out before any exec() modal starts its nested loop. Returns
// the action text (mnemonic '&' stripped). Same thread rule as postClick.
bool triggerAction(const QJsonObject& criteria, QString* triggeredText,
                   QString* error);

// Blocking wait helpers (bounded local event loop, GUI thread).
bool waitForObject(const QJsonObject& criteria, double timeoutSecs,
                   QJsonObject* found, QString* error);
bool waitForGone(const QJsonObject& criteria, double timeoutSecs,
                 QString* error);

// Domain: resolve peer by userId or display name in the main-window
// contact tree, fire the same tvUserList_itemActivated -> chatStarting
// -> startChat path as keyboard Return, wait for a visible ChatWindow.
// Returns the chat window title. Idempotent: shows the existing window.
bool openChat(const QString& peer, double timeoutSecs, QString* title,
              QString* error);

} // namespace lmc_agent

#endif // LMC_AGENT_CORE_H
