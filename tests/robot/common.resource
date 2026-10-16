*** Settings ***
Documentation     Shared keywords for the Robot Framework E2E suite.
...
...             One driver: the in-process lmc-agent (LmcLibrary.py)
...             speaking the NDJSON protocol of tools/lmc-agent/PROTOCOL.md
...             to the application under test. Menu actions, key events,
...             text entry and window closing all act on Qt objects
...             directly (Trigger Action, Type Text, Close Window), so
...             nothing depends on X focus, window ids or xdotool, and
...             exec-modal dialogs stay drivable: every agent call is
...             fire-and-post, the reply leaves before the slot runs.
Library           LmcLibrary.py
Library           Process
Library           OperatingSystem


*** Variables ***
# Overridden from CI via `-v APP_PATH:...`.
# Default points at the local dev-VM release layout. The binary must run
# with the agent active (lan-messenger-test unless LMC_TEST_AGENT=0);
# Launch LMC fails loudly when no agent answers.
${APP_PATH}       ${CURDIR}/../../lmc/release/lan-messenger-test
# The main window is shown at startup only with AutoShow=true in
# $HOME/.config/lmc/lmc.ini (lmcMainWindow::start checks it, default false).
# CI pre-seeds that file in the Seed step; never pass /noconfig here, it
# deletes the settings file.
${APP_ARGS}       /silent /loopback
${AGENT_PORT}     9999
# Seeded user name (CI seeds E2E Tester, local runs use the dev config):
# with /loopback the app lists itself under General under this name, which
# is the self-peer addresses used by the loopback suites.
${SELF_PEER}      E2E Tester
# Showcase pause: how long an opened window stays on screen before the
# test closes it, so a demo video (x11grab of the Xvfb display) shows
# each dialog instead of a millisecond flash. Default is 0s: the gate
# runs with no showcase tax, `Hold For Video` is a no-op. The demo
# workflow overrides from the command line, e.g. `-v HOLD_OPEN:3s`.
${HOLD_OPEN}      0s
# Settle delay (NOT showcase): close() on a reused modeless window hides
# it synchronously, but the X unmap and the video capture lag behind, so
# give the display a moment before the hidden assert. Stability, keep in
# gate mode.
${HIDE_SETTLE}    1s


*** Keywords ***
Launch LMC
    [Documentation]    Start the app under the in-process agent, connect and
    ...    wait until the main window is mapped. Gives the window a moment
    ...    for first paint so dumps and the CI video show real content (the
    ...    visible flag is set synchronously by show(), X mapping is async;
    ...    QTest uses qWaitForWindowExposed for the same reason).
    ...    Assumes Display/SslFixture/UserScope and LD_LIBRARY_PATH arrive
    ...    through the environment (robot never exports them itself).
    # Split, not a single argument: the app parses /silent and /loopback
    # as separate argv entries, and shell=False passes them through
    # verbatim.
    # Per-suite log name: OUTPUT_DIR is per run, and every suite launches
    # its own app — one shared lmc-app.log would be overwritten by the
    # next suite, destroying the post-mortem of a flaky one.
    @{app_args}=    Split Command Line    ${APP_ARGS}
    Start Process    ${APP_PATH}    @{app_args}    alias=app
    ...    stdout=${OUTPUT_DIR}${/}lmc-app-${SUITE NAME}.log    stderr=STDOUT
    Connect To Test Agent    ${AGENT_PORT}    timeout=30s
    Wait For Object    name=MainWindow    timeout=30
    ${visible}=      Get Object Property    name=MainWindow    visible
    Should Be True    ${visible}
    ...    MainWindow exists but is not visible (AutoShow not seeded?)
    Sleep             2s

Close LMC
    [Documentation]    Quit the app through the agent and wait for the
    ...    process to be gone: the app owns the production
    ...    single-instance socket (same appId as lan-messenger), so a
    ...    stale instance would silently absorb the next launch instead of
    ...    showing a window. Fully guarded: teardown must not mask test
    ...    results. The agent stops last, only after the process is gone.
    Run Keyword And Ignore Error    Close Application
    Run Keyword And Ignore Error    Wait For Process    app    timeout=15s
    Run Keyword And Ignore Error    Terminate Process    app
    Run Keyword And Ignore Error    Disconnect From Test Agent

Log App Liveness On Failure
    [Documentation]    Post-mortem probe for a deaf agent (the Windows
    ...    dialogs wedge: commands time out mid-suite after a healthy
    ...    start). If the test failed, report whether the app process is
    ...    still alive: APP DEAD means a probable crash, APP ALIVE with a
    ...    dead agent means a blocked event loop (e.g. a modal). Never
    ...    fails: teardown must not mask test results. The probe is Wait
    ...    For Process with 1s: finished means dead, timeout means alive.
    IF    '${TEST STATUS}' == 'FAIL'    Probe And Log App Liveness

Probe And Log App Liveness
    [Documentation]    One-shot liveness probe, see Log App Liveness On
    ...    Failure. Only called from there.
    ${finished}=    Run Keyword And Return Status
    ...    Wait For Process    app    timeout=1s
    IF    ${finished}
        Log    APP DEAD: app process finished (probable crash)    WARN
    ELSE
        Log    APP ALIVE: process runs but agent is deaf (blocked loop?)    WARN
    END

Seed App Config
    [Documentation]    Fresh lmc.ini under $HOME (caller isolates HOME per
    ...    run): AutoShow on, Version pinned to the tested binary, user
    ...    under the given name. A newer Version resets the config and
    ...    blocks startup with a modal warning. Same recipe as the CI Seed
    ...    step; never pass /noconfig here, it deletes this file. Needed
    ...    by suites that launch the app themselves (mock-peer stands);
    ...    the plain loopback suites rely on the pre-seeded HOME instead.
    [Arguments]    ${name}
    Create Directory    %{HOME}/.config/lmc
    ${ini}=    Catenate    SEPARATOR=\n
    ...    [General]
    ...    AutoShow=true
    ...    ${EMPTY}
    ...    [Application]
    ...    Version=0.0.0
    ...    ${EMPTY}
    ...    [User]
    ...    Name=${name}
    Create File    %{HOME}/.config/lmc/lmc.ini    ${ini}\n

Window Should Be Visible
    [Documentation]    Wait for a top-level window and require it mapped.
    ...    Existence alone is not enough: hidden windows are listed in the
    ...    object tree too (that once gave a vacuous PASS with black video).
    [Arguments]    ${name}
    Wait For Object    name=${name}    timeout=10
    ${visible}=      Get Object Property    name=${name}    visible
    Should Be True    ${visible}    ${name} exists but is not visible

Window Should Be Hidden
    [Documentation]    Assert a reused modeless window (Transfers, Chat Room)
    ...    was closed: these are kept alive hidden for reuse (no
    ...    WA_DeleteOnClose), so disappearance cannot be awaited.
    [Arguments]    ${window}
    Sleep             ${HIDE_SETTLE}
    ${visible}=       Get Object Property    name=${window}    visible
    Should Not Be True    ${visible}    ${window} should be hidden after close

Close Dialog
    [Documentation]    Close a dialog with a uniquely-named button (e.g.
    ...    btnCancel for Preferences) and wait until it is gone. Only use
    ...    with a button objectName that is unique across ALL windows alive
    ...    in the session, including windows hidden by earlier tests: the
    ...    agent has no parent scoping, `name=btnClose` matches the first
    ...    button with that objectName in the tree (a hidden Transfers
    ...    window once stole such a click). For shared btnClose buttons use
    ...    Close Window instead. Works for windows deleted on close and
    ...    for exec-modal dialogs, which vanish from the agent enumeration
    ...    once hidden. For reused modeless windows that only hide
    ...    (Transfers, Chat Room) use Window Should Be Hidden instead.
    [Arguments]    ${window}    ${button}
    Click Object    name=${button}
    Wait For Object To Disappear    name=${window}    timeout=10

Hold For Video
    [Documentation]    Showcase pause for the CI video: sleep ${HOLD_OPEN}
    ...    while the window under test is open, so the recording shows it
    ...    instead of a millisecond flash. Call after the asserts, right
    ...    before closing the window. Length is controlled by ${HOLD_OPEN}.
    Sleep             ${HOLD_OPEN}

Close All Chats
    [Documentation]    Close every conversation window: chatwindow.cpp, like
    ...    all lmc windows that reject on Escape, only hides behind its
    ...    children filter with close(), so Close Window runs the same path
    ...    (history is still saved by closeEvent) without the X focus
    ...    guessing that a real Escape needs. Chat windows are deleted on
    ...    close, so disappearance can be awaited, unlike reused modeless
    ...    windows.
    FOR    ${i}    IN RANGE    10
        ${open}=    Run Keyword And Return Status
        ...    Object Should Exist    name=ChatWindow visible=true
        IF    not ${open}    BREAK
        Close Window    name=ChatWindow visible=true
    END
    Wait For Object To Disappear    name=ChatWindow    timeout=10

Open Self Chat
    [Documentation]    Open a conversation with the loopback self-peer
    ...    (${SELF_PEER}) through the agent: the same chatStarting path as
    ...    keyboard Return, addressed by peer name, not by X window id
    ...    (two open chats stay distinguishable by title). Retried: the
    ...    seeded user lists itself under General only after its own
    ...    announce round-trips (/loopback, UDP+TCP to self), and the fresh
    ...    self-connection can flap once before settling (the chat sometimes
    ...    opens and auto-closes within seconds), so retry the whole open
    ...    until ChatWindow is really there.
    Wait Until Keyword Succeeds    45s    3s
    ...    Open Chat With Peer    ${SELF_PEER}    timeout=15s

Send Chat Text
    [Documentation]    Type text into the chat input and send with Return
    ...    (txtMessage sends on Enter via its event filter, chatwindow.cpp,
    ...    honoring the SendKeyMod setting: plain Return by default).
    ...    Type Text posts the key right after setting the text and
    ...    targets the visible input directly, so the keyword stays correct
    ...    with two chat windows.
    [Arguments]    ${text}
    Type Text    name=txtMessage visible=true    ${text}

Chat Log Should Contain
    [Documentation]    Assert the conversation log holds the text. The log
    ...    view is lmcMessageLog (QMessageBrowser, anonymous child of
    ...    wgtLog): matched by class, read via plainText (the agent
    ...    special-cases QTextEdit). visible=true scopes the match to the
    ...    open chat: the reused hidden ChatRoomWindow owns a second,
    ...    always-empty lmcMessageLog that would shadow the live one (P1).
    ...    times counts occurrences: a sent message appears twice (sent
    ...    line plus the loopback echo), which is the receipt proof.
    [Arguments]    ${text}    ${times}=1
    ${log}=    Get Object Property    type=lmcMessageLog visible=true    plainText
    ${n}=      Evaluate    $log.count($text)
    Should Be True    $n >= int($times)
    ...    '${text}' found ${n} time(s), expected at least ${times}

Read Visible Chat Log
    [Documentation]    Return the live conversation log text (same scoping
    ...    as Chat Log Should Contain).
    ${log}=    Get Object Property    type=lmcMessageLog visible=true    plainText
    RETURN     ${log}

Send Chat Message And Expect Echo
    [Documentation]    Send one marker and require its echo. The first send
    ...    on a fresh self-connection can land before the TCP handshake
    ...    completes and vanish silently (P1); wrapping send+assert lets the
    ...    caller retry the pair with Wait Until Keyword Succeeds. Resends
    ...    are harmless: asserts only count occurrences.
    [Arguments]    ${text}
    Send Chat Text    ${text}
    Sleep             2s
    Chat Log Should Contain    ${text}    times=2
