*** Settings ***
Documentation     File send/receive over the loopback self-peer, single
...               instance, no second user. With /loopback the app processes
...               its own announces, so the seeded user lists itself under
...               General and a conversation with it round-trips through the
...               real UDP+TCP stack (same rig as messages.robot).
...
...               Send side is keyboard-only: Ctrl+O fires the toolbar
...               "Send A File" action (its shortcut is QKeySequence::Open,
...               chatwindow.cpp), the Qt file dialog takes focus itself,
...               so the path is typed into whatever is focused + Return.
...               Never Shift+F10 here: with focus in the message input it
...               only opens the text-edit menu (proven by spike).
...
...               Accept side is keyboard-only too: the Accept/Decline links
...               live inside lmcMessageLog (QTextBrowser, not QWidgets, so
...               the agent cannot click them and click_link only targets
...               QLabel). Tab walks the anchors in document order
...               (Cancel, Accept, Decline): 3 Tabs focus Accept, 4 Tabs
...               focus Decline (proven by spike screenshots, dotted focus
...               rect). Return activates the focused anchor. If the app
...               ever reorders the log layout, recalibrate by screenshot.
...
...               Requires the usual AutoShow seed (see common.resource)
...               plus FileTransfer/StoragePath, which the Suite Setup seeds
...               itself into ~/.config/lmc/lmc.ini (default
...               ~/Documents/Received Files is XDG-fragile in CI).
Resource          common.resource
Library           OperatingSystem
Suite Setup       Launch LMC With File Storage
Suite Teardown    Close Application
Test Setup        Prepare File Test
Test Teardown     Close File Test


*** Variables ***
${PAYLOAD_BYTES}    4096
${PAYLOAD_SEED}     1234
${SEND_DIR}         /tmp/e2e-lmc-files-send
${RECV_DIR}         /tmp/e2e-lmc-files-recv


*** Test Cases ***
File Round Trip
    [Documentation]    Send a small seeded binary to self, accept with the
    ...    keyboard, wait for Completed on both sides, compare byte for
    ...    byte. The file existing with identical bytes is the receipt
    ...    proof (mirrors the md5 check of the manual two-user run).
    Open Self Chat
    Send File With Retry    ${SEND_DIR}${/}e2e-file-a.bin    e2e-file-a.bin
    Accept File Request
    Wait Until Keyword Succeeds    30s    2s
    ...    File Transfer Should Complete    e2e-file-a.bin
    ${sent}=          Get Binary File    ${SEND_DIR}${/}e2e-file-a.bin
    ${recv}=          Get Binary File    ${RECV_DIR}${/}e2e-file-a.bin
    Should Be Equal   ${sent}    ${recv}
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}files-roundtrip.json
    Hold For Video

File Decline
    [Documentation]    Send then decline: the log flips to Declined and no
    ...    file lands in the storage dir.
    Open Self Chat
    Send File With Retry    ${SEND_DIR}${/}e2e-file-b.bin    e2e-file-b.bin
    Decline File Request
    Wait Until Keyword Succeeds    15s    2s
    ...    File Decline Should Appear
    File Should Not Exist    ${RECV_DIR}${/}e2e-file-b.bin
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}files-decline.json
    Hold For Video


*** Keywords ***
Launch LMC With File Storage
    [Documentation]    Seed FileTransfer/StoragePath at ${RECV_DIR}, then
    ...    launch as usual. Seeding in-suite (not in the CI Seed step) so
    ...    the suite is self-contained: the app reads the path live at
    ...    accept time (StdLocation::fileStorageDir).
    Seed File Storage
    Launch LMC

Seed File Storage
    [Documentation]    Wipe the send/receive dirs and point the app's
    ...    storage at ${RECV_DIR} via ~/.config/lmc/lmc.ini.
    Create Directory    ${SEND_DIR}
    Empty Directory     ${SEND_DIR}
    Create Directory    ${RECV_DIR}
    Empty Directory     ${RECV_DIR}
    ${ini}=    Set Variable    %{HOME}${/}.config${/}lmc${/}lmc.ini
    ${code}=    Set Variable    import configparser,sys; p=sys.argv[1]; d=sys.argv[2]; c=configparser.ConfigParser(); c.optionxform=str; c.read(p); s='FileTransfer'; c.add_section(s) if not c.has_section(s) else 0; c.set(s,'StoragePath',d); f=open(p,'w'); c.write(f); f.close()
    ${res}=    Run Process    python3    -c    ${code}    ${ini}    ${RECV_DIR}
    Should Be Equal As Integers    ${res.rc}    0

Prepare File Test
    [Documentation]    Per-test setup: focus the main window. The payload
    ...    itself is (re)created at send time with a per-test filename
    ...    (see Send File With Retry).
    Focus Main Window

Send File To Self
    [Documentation]    One send attempt: Ctrl+O opens the file dialog,
    ...    type the full path (the dialog takes focus itself), Return
    ...    submits. xdotool, never agent keys (QTest asserts under Xvfb).
    [Arguments]    ${path}
    ${focus}=    Run Process    xdotool    windowfocus    --sync    ${CHAT_WIN}
    Should Be Equal As Integers    ${focus.rc}    0
    ${o}=        Run Process    xdotool    key    ctrl+o
    Should Be Equal As Integers    ${o.rc}    0
    Sleep             2s
    ${type}=     Run Process    xdotool    type    --delay    20    ${path}
    Should Be Equal As Integers    ${type.rc}    0
    Sleep             1s
    ${ret}=      Run Process    xdotool    key    Return
    Should Be Equal As Integers    ${ret.rc}    0

File Request Should Appear
    [Documentation]    The loopback request round-tripped: the visible log
    ...    holds the receive entry for this file.
    [Arguments]    ${filename}
    ${log}=    Get Object Property    type=lmcMessageLog visible=true    plainText
    Should Contain    ${log}    sends you a file:
    Should Contain    ${log}    ${filename}

Send File With Retry
    [Documentation]    Send until the request shows up. The first send on a
    ...    fresh self-connection can vanish before the TCP handshake
    ...    completes (same flake as messages); a vanished send leaves no
    ...    log entry, so resends never pile up pending requests.
    [Arguments]    ${path}    ${filename}
    Create Payload File    ${path}
    FOR    ${i}    IN RANGE    1    4
        Send File To Self    ${path}
        ${ok}=    Run Keyword And Return Status
        ...    Wait Until Keyword Succeeds    10s    2s
        ...    File Request Should Appear    ${filename}
        IF    ${ok}    BREAK
    END
    File Request Should Appear    ${filename}

Create Payload File
    [Arguments]    ${path}
    ${code}=    Set Variable    import random,sys; r=random.Random(${PAYLOAD_SEED}); open(sys.argv[1],'wb').write(bytes(r.randrange(256) for _ in range(${PAYLOAD_BYTES})))
    ${res}=    Run Process    python3    -c    ${code}    ${path}
    Should Be Equal As Integers    ${res.rc}    0
    File Should Exist    ${path}

Accept File Request
    [Documentation]    Focus the chat, Tab to the Accept anchor (Cancel,
    ...    Accept, Decline in document order), Return activates it. The
    ...    transfer then runs without further input.
    Focus Chat Window By Id
    Send Key To Chat    Tab
    Send Key To Chat    Tab
    Send Key To Chat    Tab
    Send Key To Chat    Return

Decline File Request
    [Documentation]    Same walk one step further: Decline instead.
    Focus Chat Window By Id
    Send Key To Chat    Tab
    Send Key To Chat    Tab
    Send Key To Chat    Tab
    Send Key To Chat    Tab
    Send Key To Chat    Return

Focus Chat Window By Id
    ${res}=    Run Process    xdotool    windowfocus    --sync    ${CHAT_WIN}
    Should Be Equal As Integers    ${res.rc}    0

Send Key To Chat
    [Documentation]    One X key to the focused chat window.
    [Arguments]    ${keys}
    ${res}=    Run Process    xdotool    key    ${keys}
    Should Be Equal As Integers    ${res.rc}    0
    Sleep             1s

File Transfer Should Complete
    [Documentation]    Both the send and receive log entries flipped to
    ...    Completed and the received file is on disk (polled).
    [Arguments]    ${filename}
    Chat Log Should Contain    Completed    times=2
    File Should Exist    ${RECV_DIR}${/}${filename}

File Decline Should Appear
    [Documentation]    The receive entry flipped to Declined (polled).
    Chat Log Should Contain    Declined

Close File Test
    [Documentation]    Close chats, then close the auto-popped Transfers
    ...    window if the transfer opened it (AutoShowFile defaults true).
    Close All Chats
    Dismiss Transfers If Open

Dismiss Transfers If Open
    ${res}=    Run Process    xdotool    search    --onlyvisible    --name    File Transfers
    IF    ${res.rc} == 0
        Dismiss Dialog    File Transfers
        Window Should Be Hidden    TransferWindow
    END
