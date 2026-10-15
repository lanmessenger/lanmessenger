*** Settings ***
Documentation     File send/receive over the loopback self-peer, single
...               instance, no second user. With /loopback the app processes
...               its own announces, so the seeded user lists itself under
...               General and a conversation with it round-trips through the
...               real UDP+TCP stack (same rig as messages.robot).
...
...               Everything goes through the in-process lmc-agent (see
...               todo/lmc-test-adapter.md): the chat opens with Open Chat
...               With Peer (same chatStarting path as keyboard Return),
...               the send fires the toolbar "Send A File..." QAction and
...               the path goes into the (non-native) QFileDialog's
...               fileNameEdit + Return. The dialog is exec-modal, but an
...               agent trigger queues past the reply, so the connection
...               survives and the dialog can be driven.
...
...               Accept/Decline are anchors inside lmcMessageLog
...               (QTextBrowser, not QWidgets, so they cannot be clicked as
...               objects): Tab walks the document-order anchors (Cancel,
...               Accept, Decline). The first Tab goes to the input, whose
...               tabChangesFocus (chatwindow.ui) hands focus to the log;
...               further Tabs move between anchors. Return activates the
...               focused anchor. If the app ever reorders the log layout,
...               recalibrate the Tab counts by screenshot.
...
...               Requires the usual AutoShow seed (see common.resource)
...               plus FileTransfer/StoragePath, which the Suite Setup seeds
...               itself into ~/.config/lmc/lmc.ini (default
...               ~/Documents/Received Files is XDG-fragile in CI).
Resource          common.resource
Library           OperatingSystem
Library           LmcLibrary.py    WITH NAME    LmcAgent
Suite Setup       Launch LMC With File Storage And Agent
Suite Teardown    Close LMC With Test Agent
Test Teardown     Close File Test


*** Variables ***
${PAYLOAD_BYTES}    4096
${PAYLOAD_SEED}     1234
${SEND_DIR}         /tmp/e2e-lmc-files-send
${RECV_DIR}         /tmp/e2e-lmc-files-recv
# Our preload .so rides along with qtexpert's (its launcher prepends to
# LD_PRELOAD instead of replacing it) on its own port. Overridable via -v.
${TEST_AGENT_SO}    ${CURDIR}/../../build/tests/lmc-agent/liblmc_test_preload.so
${TEST_AGENT_PORT}    9999
# Seeded user name (CI seeds E2E Tester, local runs use the dev config):
# with /loopback the app lists itself under General under this name.
${SELF_PEER}      E2E Tester


*** Test Cases ***
File Round Trip
    [Documentation]    Send a small seeded binary to self, accept with the
    ...    keyboard, wait for Completed on both sides, compare byte for
    ...    byte. The file existing with identical bytes is the receipt
    ...    proof (mirrors the md5 check of the manual two-user run).
    Open Self Chat Via Agent
    Send File With Retry    ${SEND_DIR}${/}e2e-file-a.bin    e2e-file-a.bin
    AcceptFileRequest
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
    Open Self Chat Via Agent
    Send File With Retry    ${SEND_DIR}${/}e2e-file-b.bin    e2e-file-b.bin
    DeclineFileRequest
    Wait Until Keyword Succeeds    15s    2s
    ...    File Decline Should Appear
    File Should Not Exist    ${RECV_DIR}${/}e2e-file-b.bin
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}files-decline.json
    Hold For Video


*** Keywords ***
Launch LMC With File Storage And Agent
    [Documentation]    Seed FileTransfer/StoragePath, then launch the app
    ...    with our preload agent connected. Seeding in-suite (not in the
    ...    CI Seed step) so the suite is self-contained: the app reads the
    ...    path live at accept time (StdLocation::fileStorageDir).
    Seed File Storage
    Set Environment Variable    LD_PRELOAD    ${TEST_AGENT_SO}
    Set Environment Variable    LMC_AGENT_PORT    ${TEST_AGENT_PORT}
    Launch LMC
    LmcAgent.Connect To Test Agent    ${TEST_AGENT_PORT}    timeout=30s

Close LMC With Test Agent
    Run Keyword And Ignore Error    LmcAgent.Disconnect From Test Agent
    Run Keyword And Ignore Error    Close Application

Open Self Chat Via Agent
    [Documentation]    Open the self-peer conversation through the agent
    ...    (same chatStarting path as keyboard Return, idempotent).
    ...    Retried: the seeded user lists itself under General only after
    ...    its own announce round-trips (/loopback, UDP+TCP to self), and
    ...    the fresh self-connection can flap once before settling (the
    ...    chat sometimes opens and auto-closes within seconds), so retry
    ...    the whole open until ChatWindow is really there.
     Wait Until Keyword Succeeds    45s    3s
     ...    LmcAgent.Open Chat With Peer    ${SELF_PEER}    timeout=15s

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

Send File Via Agent
    [Documentation]    One send attempt: trigger the "Send A File..."
    ...    toolbar action (fires btnFile_clicked -> QFileDialog), type the
    ...    full path into the dialog's fileNameEdit, Return submits. The
    ...    dialog is the non-native QFileDialog (visible in the object tree
    ...    with name=fileNameEdit), so it can be driven by the agent; the
    ...    trigger is fire-and-post, so the exec-modal dialog does not
    ...    swallow the connection.
    [Arguments]    ${path}
    LmcAgent.Agent Trigger Action    text="Send A File..."
    LmcAgent.Agent Wait For Object    type=QFileDialog visible=true    timeout=10s
    LmcAgent.Agent Type Text    name=fileNameEdit    ${path}    send=False
    LmcAgent.Agent Send Key    Return    locator=name=fileNameEdit

File Request Should Appear
    [Documentation]    The loopback request round-tripped: the visible log
    ...    holds the receive entry for this file.
    [Arguments]    ${filename}
    ${log}=    Read Visible Chat Log
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
        Send File Via Agent    ${path}
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

Activate File Anchor
    [Documentation]    Focus the ${anchor}-th anchor of the file request
    ...    entry and activate it with Return. The first Tab leaves the
    ...    message input (tabChangesFocus hands focus to the log), the
    ...    rest walk the document-order anchors (Cancel, Accept, Decline).
    ...    Calibrated by screenshot; Recacalibrate if the log layout
    ...    changes.
    [Arguments]    ${anchor}
    LmcAgent.Agent Send Key    Tab    locator=name=txtMessage visible=true
    FOR    ${i}    IN RANGE    ${anchor}
        LmcAgent.Agent Send Key    Tab    locator=type=lmcMessageLog visible=true
    END
    LmcAgent.Agent Send Key    Return    locator=type=lmcMessageLog visible=true

AcceptFileRequest
    [Documentation]    Walk the anchors to Accept and activate it: 1 tab out
    ...    of the input + 2 anchor tabs (Cancel, Accept). The transfer then
    ...    runs without further input.
    Activate File Anchor    2

DeclineFileRequest
    [Documentation]    Same walk one anchor further: Decline instead (input
    ...    tab + Cancel, Accept, Decline).
    Activate File Anchor    3

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
    Close Chats Via Agent
    Dismiss Transfers If Open

Close Chats Via Agent
    [Documentation]    Escape every visible chat through the agent: the key
    ...    goes straight to the message log, whose eventFilter closes the
    ...    window (chatwindow.cpp installs it on the log and on the input).
    ...    Posting the key directly matters here: the accept walk leaves the
    ...    focus off the input (a Tab in txtMessage has tabChangesFocus), so
    ...    an X-delivered Escape would land on the wrong widget and the
    ...    chat would stay open. Chats are deleted on close, so the final
    ...    wait observes a real disappearance.
    FOR    ${i}    IN RANGE    10
        ${visible}=    Run Keyword And Return Status
        ...    LmcAgent.Agent Object Should Exist    name=ChatWindow visible=true
        IF    not ${visible}    BREAK
        LmcAgent.Agent Send Key    Escape    locator=type=lmcMessageLog visible=true
        Sleep             0.5s
    END
    LmcAgent.Agent Wait For Object To Disappear    name=ChatWindow    timeout=10s

Dismiss Transfers If Open
    [Documentation]    Escape the Transfers window only when it is actually
    ...    visible: it is a hidden-reused window (no WA_DeleteOnClose), so
    ...    existence alone would close nothing and the hidden assert would
    ...    pass vacuously. The Escape goes to the transfer list: the
    ...    window's eventFilter lives on its children, not on the window
    ...    itself (transferwindow.cpp).
    ${visible}=    Run Keyword And Return Status
    ...    LmcAgent.Agent Object Should Exist    name=TransferWindow visible=true
    IF    ${visible}
        LmcAgent.Agent Send Key    Escape    locator=name=lvTransferList
        Sleep             1s
        ${hidden}=    LmcAgent.Agent Get Object Property    name=TransferWindow    visible
        Should Not Be True    ${hidden}    TransferWindow should be hidden after close
    END
