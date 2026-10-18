*** Settings ***
Documentation     File send/receive over the loopback self-peer, single
...               instance, no second user. With /loopback the app processes
...               its own announces, so the seeded user lists itself under
...               General and a conversation with it round-trips through the
...               real UDP+TCP stack (same rig as chat.loopback.robot).
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
Library           String
Suite Setup       Launch LMC With File Storage
Suite Teardown    Close LMC
Test Teardown     Close File Test


*** Variables ***
${PAYLOAD_BYTES}    4096
${PAYLOAD_SEED}     1234
# TEMPDIR, not hardcoded /tmp: on Windows /tmp is drive-relative mush and
# the typed path never resolves in the file dialog (silent empty submit —
# no send entry, no error). TEMPDIR keeps native separators on every OS.
${SEND_DIR}         ${TEMPDIR}${/}e2e-lmc-files-send
${RECV_DIR}         ${TEMPDIR}${/}e2e-lmc-files-recv


*** Test Cases ***
File Round Trip
    [Documentation]    Send a small seeded binary to self, accept with the
    ...    keyboard, wait for Completed on both sides, compare byte for
    ...    byte. The file existing with identical bytes is the receipt
    ...    proof (mirrors the md5 check of the manual two-user run).
    Open Self Chat
    Send File With Retry    ${SEND_DIR}${/}e2e-file-a.bin    e2e-file-a.bin
    AcceptFileRequest
    Wait Until Keyword Succeeds    30s    2s
    ...    File Transfer Should Complete    e2e-file-a.bin
    ${sent}=          Get Binary File    ${SEND_DIR}${/}e2e-file-a.bin
    ${recv}=          Get Binary File    ${RECV_DIR}${/}e2e-file-a.bin
    Should Be Equal   ${sent}    ${recv}
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}files-roundtrip.json

File Decline
    [Documentation]    Send then decline: the log flips to Declined and no
    ...    file lands in the storage dir.
    Open Self Chat
    Send File With Retry    ${SEND_DIR}${/}e2e-file-b.bin    e2e-file-b.bin
    DeclineFileRequest
    Wait Until Keyword Succeeds    15s    2s
    ...    File Decline Should Appear
    File Should Not Exist    ${RECV_DIR}${/}e2e-file-b.bin
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}files-decline.json


*** Keywords ***
Launch LMC With File Storage
    [Documentation]    Seed FileTransfer/StoragePath, then launch the app
    ...    under the agent. Seeding in-suite (not in the CI Seed step) so
    ...    the suite is self-contained: the app reads the path live at
    ...    accept time (StdLocation::fileStorageDir).
    ...    Identity first: Seed File Storage edits the ini in place and
    ...    preserves whatever is there — on a fresh HOME (Windows) that
    ...    would be identity-less, and the self-peer lookup needs Name.
    Seed App Config    ${SELF_PEER}
    Seed File Storage
    Launch LMC

Seed File Storage
    [Documentation]    Wipe the send/receive dirs and point the app's
    ...    storage at ${RECV_DIR} via FileTransfer/StoragePath. Seeded
    ...    into every ini the app may read: %{HOME}/.config on Unix, and
    ...    %APPDATA% on Windows (the Windows app never looks at HOME).
    Create Directory    ${SEND_DIR}
    Empty Directory     ${SEND_DIR}
    Create Directory    ${RECV_DIR}
    Empty Directory     ${RECV_DIR}
    Seed Storage Path In    %{HOME}${/}.config${/}lmc${/}lmc.ini
    # NOTE: $appdata (object form), never '${appdata}': ${}-substitution
    # puts a raw Windows path (C:\...) into the expression and the parser
    # chokes on \U-escapes (unicodeescape SyntaxError). $var passes the
    # object and never re-parses it.
    ${appdata}=    Get Environment Variable    APPDATA    default=${EMPTY}
    IF    $appdata != ''
        Seed Storage Path In    ${appdata}${/}LAN Messenger${/}LAN Messenger.ini
    END

Seed Storage Path In
    [Documentation]    Patch FileTransfer/StoragePath into one ini file in
    ...    place, preserving whatever identity the CI seed put there.
    ...    The path goes in with forward slashes: QSettings IniFormat
    ...    treats backslash as an escape on read (C:\Users\... comes back
    ...    as C:sers... and the receiver mkpaths the garbage — Completed
    ...    in the log, no file where expected), while Qt, Python and RF
    ...    all accept forward slashes on Windows.
    [Arguments]    ${ini}
    ${posix}=    Replace String    ${RECV_DIR}    \\    /
    ${code}=    Set Variable    import configparser,sys; p=sys.argv[1]; d=sys.argv[2]; c=configparser.ConfigParser(); c.optionxform=str; c.read(p); s='FileTransfer'; c.add_section(s) if not c.has_section(s) else 0; c.set(s,'StoragePath',d); f=open(p,'w'); c.write(f); f.close()
    ${res}=    Run Process    python3    -c    ${code}    ${ini}    ${posix}
    Should Be Equal As Integers    ${res.rc}    0

Send File
    [Documentation]    One send attempt: trigger the "Send A File..."
    ...    toolbar action (fires btnFile_clicked -> QFileDialog), type the
    ...    full path into the dialog's fileNameEdit, Return submits. The
    ...    dialog must be the Qt non-native QFileDialog (visible in the
    ...    object tree with name=fileNameEdit): Qt static file dialogs
    ...    default to the platform-native panel (IFileDialog on Windows,
    ...    NSOpenPanel on macOS — both invisible to the agent), so the
    ...    harness exports LMC_NON_NATIVE_DIALOGS=1 on those legs, which
    ...    btnFile_clicked honors with DontUseNativeDialog; on Linux the
    ...    fallback is non-native already. The trigger is fire-and-post,
    ...    so the exec-modal dialog does not swallow the connection.
    [Arguments]    ${path}
    Trigger Action    text="Send A File..."
    Wait For Object    type=QFileDialog visible=true    timeout=10s
    Type Text    name=fileNameEdit    ${path}    send=False
    Send Key    Return    locator=name=fileNameEdit

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
    ...    log entry, so resends never pile up pending requests. Two-phase:
    ...    the send-side `Sending '...' to` entry proves the dialog
    ...    submitted (no send entry means the path never reached sendFile);
    ...    the receive entry proves the loopback delivery.
    [Arguments]    ${path}    ${filename}
    Create Payload File    ${path}
    FOR    ${i}    IN RANGE    1    4
        Send File    ${path}
        ${sent}=    Run Keyword And Return Status
        ...    Wait Until Keyword Succeeds    10s    2s
        ...    Chat Log Should Contain    Sending '${filename}' to
        IF    not ${sent}    CONTINUE
        ${ok}=    Run Keyword And Return Status
        ...    Wait Until Keyword Succeeds    10s    2s
        ...    File Request Should Appear    ${filename}
        IF    ${ok}    BREAK
    END
    Chat Log Should Contain    Sending '${filename}' to
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
    Send Key    Tab    locator=name=txtMessage visible=true
    FOR    ${i}    IN RANGE    ${anchor}
        Send Key    Tab    locator=type=lmcMessageLog visible=true
    END
    Send Key    Return    locator=type=lmcMessageLog visible=true

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
    Close All Chats
    Dismiss Transfers If Open

Dismiss Transfers If Open
    [Documentation]    Transfers is a hidden-reused window (no
    ...    WA_DeleteOnClose), so closing it only hides it: assert hidden
    ...    instead of waiting for it to disappear.
    ${visible}=    Run Keyword And Return Status
    ...    Object Should Exist    name=TransferWindow visible=true
    IF    ${visible}
        Close Window    name=TransferWindow
        Window Should Be Hidden    TransferWindow
    END
