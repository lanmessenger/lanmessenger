*** Settings ***
Documentation     Shared keywords for the Robot Framework E2E suite.
...
...             Two drivers are combined on purpose:
...             - the qtexpert C++ agent (LD_PRELOAD, mode=preload) owns app
...               lifecycle, waits, property asserts and UI-tree dumps;
...             - xdotool sends real X key events for menu shortcuts and
...               mnemonics. The agent cannot do these: QTest key simulation
...               hangs (agent issue, proven by spike), QAction menu items are
...               not QWidgets (invisible to findWidget), and an agent click
...               that opens an exec-modal dialog blocks the agent connection.
...               Real keys keep the connection alive, so modals opened this
...               way can still be driven with agent keywords.
Library           qtexpert    mode=preload
Library           Process


*** Variables ***
# Overridden from CI via `-v APP_PATH:...` / `-v AGENT_SO:...`.
# Defaults point at the local dev-VM layout.
${APP_PATH}       ${CURDIR}/../../lmc/release/lan-messenger
${AGENT_SO}       ${CURDIR}/../../build/qtexpert-agent/libqt_test_agent.so
# The main window is shown at startup only with AutoShow=true in
# $HOME/.config/lmc/lmc.ini (lmcMainWindow::start checks it, default false).
# CI pre-seeds that file in the Seed step; never pass /noconfig here, it
# deletes the settings file. Mirrors tests/e2e/tst_lmc_gui.cpp.
${APP_ARGS}       /silent /loopback
${AGENT_PORT}     9988
# Showcase pause: how long an opened window stays on screen before the
# test closes it, so the CI video (x11grab of the Xvfb display) shows
# each dialog instead of a millisecond flash. Overridable from the
# command line, e.g. `-v HOLD_OPEN:5s`; CI passes its own value.
${HOLD_OPEN}      3s


*** Keywords ***
Launch LMC
    [Documentation]    Start the app under the agent and wait until the main
    ...    window is mapped. Gives the window a moment for first paint so
    ...    dumps and the CI video show real content (the visible flag is set
    ...    synchronously by show(), X mapping is async; QTest uses
    ...    qWaitForWindowExposed for the same reason).
    Start Application With Qt Agent    ${APP_PATH}    ${AGENT_SO}
    ...    arguments=${APP_ARGS}
    ...    port=${AGENT_PORT}
    Wait For Object    name=MainWindow    timeout=30
    ${visible}=      Get Object Property    name=MainWindow    visible
    Should Be True    ${visible}    MainWindow exists but is not visible (AutoShow not seeded?)
    Sleep             3s
    Focus Main Window

Focus Main Window
    [Documentation]    Give the main window real X input focus. windowfocus is
    ...    used instead of windowactivate: there is no window manager under
    ...    Xvfb, so _NET_ACTIVE_WINDOW requests are refused. Remembers the
    ...    window id in ${MAIN_WIN} for Send Key.
    ${res}=    Run Process    xdotool    search    --onlyvisible    --name    LAN Messenger
    Should Be Equal As Integers    ${res.rc}    0
    ${win}=    Evaluate    """${res.stdout}""".split()[0]
    Set Suite Variable    ${MAIN_WIN}    ${win}
    ${focus}=    Run Process    xdotool    windowfocus    --sync    ${win}
    Should Be Equal As Integers    ${focus.rc}    0

Send Key
    [Documentation]    Send a real X key event to the main window, e.g.
    ...    `ctrl+h` (History), `ctrl+j` (Transfers), `ctrl+n` (chat room),
    ...    `F1` (Help) or menu mnemonics (`alt+t`, then `p`). Call
    ...    Focus Main Window first (Launch LMC already does).
    [Arguments]    ${keys}
    ${res}=    Run Process    xdotool    key    ${keys}
    Should Be Equal As Integers    ${res.rc}    0

Window Should Be Visible
    [Documentation]    Wait for a top-level window and require it mapped.
    ...    Existence alone is not enough: hidden windows are listed in the
    ...    object tree too (that once gave a vacuous PASS with black video).
    [Arguments]    ${name}
    Wait For Object    name=${name}    timeout=10
    ${visible}=      Get Object Property    name=${name}    visible
    Should Be True    ${visible}    ${name} exists but is not visible

Close Dialog
    [Documentation]    Close a dialog with a uniquely-named button (e.g.
    ...    btnCancel for Preferences) and wait until it is gone. Only use
    ...    with a button objectName that is unique across ALL windows alive
    ...    in the session, including windows hidden by earlier tests: the
    ...    agent has no parent scoping, `name=btnClose` matches the first
    ...    button with that objectName in the tree (a hidden Transfers
    ...    window once stole such a click). For shared btnClose buttons use
    ...    Dismiss Dialog instead. Works for windows deleted on close and
    ...    for exec-modal dialogs, which vanish from the agent enumeration
    ...    once hidden. For reused modeless windows that only hide
    ...    (Transfers, Chat Room) use Window Should Be Hidden instead.
    [Arguments]    ${window}    ${button}
    Click Object    name=${button}
    Wait For Object To Disappear    name=${window}    timeout=10

Dismiss Dialog
    [Documentation]    Close the dialog with the given window title via a
    ...    real Escape key (all lmc dialogs reject on Esc). Focusing by
    ...    title avoids the ambiguous shared `btnClose` objectName. Does
    ...    not wait: follow with a disappear or hidden assert.
    [Arguments]    ${title}
    ${res}=       Run Process    xdotool    search    --onlyvisible    --name    ${title}
    Should Be Equal As Integers    ${res.rc}    0
    ${win}=       Evaluate    """${res.stdout}""".split()[0]
    ${esc}=       Run Process    xdotool    windowfocus    --sync    ${win}    key    Escape
    Should Be Equal As Integers    ${esc.rc}    0

Hold For Video
    [Documentation]    Showcase pause for the CI video: sleep ${HOLD_OPEN}
    ...    while the window under test is open, so the recording shows it
    ...    instead of a millisecond flash. Call after the asserts, right
    ...    before closing the window. Length is controlled by ${HOLD_OPEN}.
    Sleep             ${HOLD_OPEN}

Window Should Be Hidden
    [Documentation]    Assert a reused modeless window (Transfers, Chat Room)
    ...    was closed: these are kept alive hidden for reuse (no
    ...    WA_DeleteOnClose), so disappearance cannot be awaited.
    [Arguments]    ${window}
    Sleep             2s
    ${visible}=       Get Object Property    name=${window}    visible
    Should Not Be True    ${visible}    ${window} should be hidden after close
