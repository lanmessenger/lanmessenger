# lmc-agent wire protocol (v1)

In-process GUI adapter for the LAN Messenger E2E suite. Replaces
`robotframework-qtexpert` + `xdotool` (see `todo/lmc-test-adapter.md`).

Transport: `QTcpSocket`/`QTcpServer` on `127.0.0.1`, port from env
`LMC_AGENT_PORT` (default `9999`). The server lives in the application's
GUI thread: every command runs there, so no cross-thread widget access.

Framing: newline-delimited JSON, UTF-8. One line = one message, `\n`
terminated. Same envelope style as `tools/lmm/PROTOCOL.md`.

## Requests

```json
{"id": 1, "cmd": "open_chat", "params": {"peer": "MockBob"}}
```

- `id`: client-chosen (number or string), echoed back.
- `cmd`: one of the commands below.
- `params`: object, may be omitted.

## Responses

```json
{"id": 1, "ok": true, "result": {"title": "MockBob - Conversation"}}
{"id": 2, "ok": false, "error": "peer not found: Nobody"}
```

Malformed lines get `{"id": 0, "ok": false, "error": "invalid json"}`.
Unknown commands get `{"ok": false, "error": "unknown cmd: ..."}`.

## Locators

Selector syntax (kept qtexpert-compatible so migrated suites read the
same):

- `name=MainWindow` (alias `objectName=`), `type=QPushButton`
  (aliases `class=`, `className=`), `text=Send A File`
  (alias `title=`), `window=History` (alias `windowTitle=`),
  `visible=true`, `enabled=true`.
- Bare string without `=` means `name=` (objectName match).
- Values with spaces need quoting: `text="File Transfers"`
  (single quotes work too); unquoted values stop at whitespace.
- Search covers `QApplication::topLevelWidgets()` plus
  `findChildren<QWidget*>()` **plus** `actions()` of
  `QMenuBar`/`QToolBar`/`QMenu`/`QToolButton` (menus are `QAction`s, not
  `QWidget`s, so the qtexpert agent could not see them at all). The
  `visible` filter applies inside `find`.

## Execution model: fire-and-post

No synchronous `QTest::mouseClick`/`keyClick` (that hangs the server on
`exec()` modals and asserts under Xvfb, see `done/spix-evaluation.md`):

- `click`/`send_key` post `QMouseEvent`/`QKeyEvent` (press+release) and
  return immediately.
- Triggers that open `exec()` modals go through
  `QTimer::singleShot(0, ...)` — the reply is sent before the nested
  loop starts, so the connection stays alive.
- Commands that must observe a result (`wait_for_object`,
  `wait_for_gone`, `open_chat`) run a bounded local `QEventLoop`
  (default 10s, overridable via `timeout`) and then reply. While the
  local loop runs, the server is `busy`: a second pipelined request gets
  `{"ok": false, "error": "busy"}`. The Robot library never pipelines
  (one request at a time), so this is a guard, not a limit.

## Commands (v1)

| cmd | params | result |
|---|---|---|
| `wait_for_object` | `{locator, timeout?}` | `{found: true, class, objectName, visible}` or error `not found within Ns` |
| `wait_for_gone` | `{locator, timeout?}` | `{gone: true}` or error `still present after Ns (class=... title="..." visible=... sameWindow=...)` — `sameWindow=false` means the object seen at entry is gone but a new one matches now (recreated); `visible=false` means close hid it but deletion never landed |
| `object_exists` | `{locator}` | `{exists: bool}` |
| `get_property` | `{locator, property}` | `{value: ...}`. `text`/`plainText` on a `QTextEdit`/`QPlainTextEdit`/`QLineEdit` reads its content (qtexpert parity: Qt has no `text` dynamic property on a `QTextEdit`, and the chat log is a `QTextEdit` subclass); otherwise a plain Qt dynamic property |
| `set_property` | `{locator, property, value}` | `{set: true}` |
| `click` | `{locator}` | `{clicked: true}` (async post; for `QAction` — async `trigger()`) |
| `trigger_action` | `{locator}` | `{triggered: "<action text>"}` — `QAction` only (menus/toolbars, matched by `text=` with `&` stripped, e.g. `text=History`); async `trigger()` via `singleShot(0)`, so an `exec()` modal opened by the slot cannot swallow the reply. Leaf commands win over same-named submenu titles (e.g. the `Help` item over the `Help` menu itself, whose trigger would only pop the dropdown): `topLevelWidgets()` order is not stable, so first-match would be a coin flip |
| `dump_tree` | `{}` | `{tree: [...]}` — top-level widgets with children and actions (`objectName`, `class`, `visible`, `title`/`text`). The client library writes the `${OUTPUT_DIR}/*.json` files |
| `get_log` | `{}` | `{path}` — this instance's trace file; empty when tracing is off (launch with `/trace`) |
| `list_windows` | `{}` | `{windows: [{objectName, class, title, visible}]}` |
| `grab_screenshot` | `{locator?, file}` | `{file}` — rendered app-side via `QWidget::grab` |
| `open_chat` | `{peer, timeout?}` | `{title}` — resolve `peer` by name or userId in the main-window contact tree, fire the same `tvUserList_itemActivated → chatStarting → startChat` path as keyboard Return (idempotent: reopens/shows the existing window), wait for a visible `ChatWindow`, return its title |
| `close_app` | `{}` | `{closing: true}` — async `QCoreApplication::quit`, reply first |
| `type_text` | `{locator, text, send?}` | `{typed: true}` — set text on a `QTextEdit`/`QPlainTextEdit`/`QLineEdit` directly; with `send=true` (default) a `Return` press is posted right after, so the widget's own Enter handling runs (e.g. `ChatWindow::eventFilter → sendMessage`, honoring the `SendKeyMod` setting) |
| `send_key` | `{locator?, key, modifiers?}` | `{sent: "<key>"}` — post press+release to the widget (or the focus widget / active window when `locator` is omitted). `key`: `Return Enter Tab Backtab Escape(Esc) Space Backspace Delete Home End Left Right Up Down` (case-insensitive). `modifiers`: array of `ctrl/control shift alt meta`. No printable characters — text entry goes through `type_text` |
| `close_window` | `{locator}` | `{objectName, class, title, visible, isWindow, ptr}` — `QWidget::close()` on the top-level window matching `locator` (a child locator resolves to `QWidget::window()`), async `singleShot(0)`. The close itself is async, so `visible`/`isWindow`/`ptr` describe the post snapshot: `ptr` correlates this exact object with a later `wait_for_gone sameWindow` verdict. Deterministic counterpart of Escape: every lmc window that rejects on Escape installs its filter on **children** and the handler is only `close()` (`chatwindow.cpp`, `historywindow.cpp`, `helpwindow.cpp`, `transferwindow.cpp`), so an X-delivered Escape depends on the focus widget while this does not. Modeless reuse windows (Transfers, Chat Room) only hide; `WA_DeleteOnClose` windows are deleted |
| `open_window` | — | stub: `not implemented (phase 2)` |
| `type_path` | — | stub: `not implemented (phase 2)` |

`timeout` is seconds (number or `"10s"` string, Robot-style).

## Post-mortem flow

On GUI-subsystem OSes stdout is detached, so the trace file is the only
app-side record. Query `get_log` while the agent is healthy; in teardown
read the file from disk (a wedged agent answers nothing, the file stays
readable).

## Notes / v1 limits

- `open_chat` matches contacts by userId first, then by display name
  (same precedence as the app's own lookups). The contact tree lives in
  `tvUserList` (`mainwindow.ui`); item roles follow
  `lmc/src/core/uidefinitions.h` (`IdRole = UserRole+1`,
  `TypeRole = UserRole+2`, `"User"` marks a contact row).
- Addressing is by peer name/userId, never by X window id — two open
  chats stay distinguishable by title.
- One server per process. `lan-messenger-test` carries the agent linked
  in directly (same `lmc_core` startup as production via `lmcRunApplication`
  with a pre-exec hook): same protocol, same port source
  (`LMC_AGENT_PORT`, default 9999). `LMC_TEST_AGENT=0` disables the
  server for production-like runs.
- The agent emits no X events (no focus stealing, no window ids), so
  suites run under Xvfb without a window manager or xdotool.
