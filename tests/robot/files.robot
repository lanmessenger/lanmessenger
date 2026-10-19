*** Settings ***
Documentation     File size sweep app -> mock over the real network stack.
...               The regression net for issue #3 (sender shows Completed
...               while the receiver gets a truncated file with Interrupted).
...
...               The app sends payloads of many sizes to the mock peer
...               through the GUI (same toolbar "Send A File..." + QFileDialog
...               path as files.loopback.robot); the mock accepts via a single
...               RPC (no Tab-anchor navigation) and stores into its own
...               Received Files dir. Proof is byte equality: sent file vs
...               the path reported in the mock's file.completed event.
...               Before the FileSender flush fix the large rows fail: the mock
...               emits file.failed with a short file instead of
...               file.completed, and the app log still flips to Completed.
...               Chunk is 64K (bufferSize 65535, netstreamer.cpp): the ladder
...               below pins exact-chunk, chunk+1 and ragged-tail sizes so a
...               boundary-logic breakage shows independently of the flush bug.
Resource          common.resource
Resource          mockstand.resource
Library           OperatingSystem
Suite Setup       Start File Stand
Suite Teardown    Stop File Stand
Test Teardown     Close All Chats


*** Variables ***
${LMM_PORT}       17772
${MOCK}           MockBob
${APP_NAME}       E2E Tester
# Normal network mode: /loopback would make the app talk to itself
# instead of discovering the mock (see messagingproc.cpp self-filter).
${APP_ARGS}       /silent
${SEND_DIR}       ${TEMPDIR}${/}e2e-lmc-files-mock-send


*** Keywords ***
Start File Stand
    [Documentation]    Daemon + real app up, mock added, cross-visibility
    ...    confirmed on the mock side, event cursor reset for delivery.
    Seed App Config    ${APP_NAME}
    Start Mock Stand    ${LMM_PORT}    ${OUTPUT_DIR}/lmmd-home
    Launch LMC
    Add Peer    ${MOCK}
    Wait For Contact Ready    ${MOCK}    ${APP_NAME}    timeout=30s
    Reset Event Cursor

Stop File Stand
    [Documentation]    App first (its Depart still goes out), then daemon.
    ...    Everything guarded: teardown must not mask test results.
    Run Keyword And Ignore Error    Close LMC
    Run Keyword And Ignore Error    Stop Mock Stand

Open Chat With Mock Once
    [Documentation]    One open attempt (see chat.robot): resolve the mock
    ...    contact and fire the real chatStarting path.
    Close All Chats
    Open Chat With Peer    ${MOCK}
    Window Should Be Visible    ChatWindow

Open Chat With Mock
    [Documentation]    Retried open: the mock row appears only after the app
    ...    processes its announce (seconds).
    Wait Until Keyword Succeeds    45s    3s    Open Chat With Mock Once

Send File Via Dialog
    [Documentation]    Trigger "Send A File...", type the full path into the
    ...    (non-native) QFileDialog and submit with Return. Same path as
    ...    files.loopback.robot; the harness exports LMC_NON_NATIVE_DIALOGS
    ...    on legs with native panels (see e2e.yml).
    [Arguments]    ${path}
    Trigger Action    text="Send A File..."
    Wait For Object    type=QFileDialog visible=true    timeout=10s
    Type Text    name=fileNameEdit    ${path}    send=False
    Send Key    Return    locator=name=fileNameEdit

Create Payload
    [Documentation]    ${kb} whole KB plus ${extra} bytes (total
    ...    kb*1024+extra) of os.urandom: instant (no per-byte Python loop),
    ...    non-deterministic but the assert compares sent vs received
    ...    bytes, not a fixed hash. Chunk is 65535 B, so exact-chunk sizes
    ...    are reachable: 65535 = 63KB+1023, 131070 = 127KB+1022.
    [Arguments]    ${path}    ${kb}=8192    ${extra}=0
    Create Directory    ${SEND_DIR}
    ${bytes}=    Evaluate    int($kb) * 1024 + int($extra)
    ${code}=    Set Variable    import sys; open(sys.argv[1],'wb').write(__import__('os').urandom(int(sys.argv[2])))
    ${res}=    Run Process    python3    -c    ${code}    ${path}    ${bytes}
    Should Be Equal As Integers    ${res.rc}    0
    ${actual}=    Get File Size    ${path}
    Should Be Equal As Integers    ${actual}    ${bytes}

App Sends File Mock Receives
    [Documentation]    Shared app -> mock flow: GUI send of ${filename}
    ...    (${kb}KB+${extra}B), mock RPC accept, byte equality between the
    ...    sent file and the mock's stored copy. ${timeout} sizes the
    ...    file.completed wait. Distinct ${filename} per row: the mock
    ...    suffixes collisions (" [1]") via getFreeFileName, which would
    ...    confuse the fileName matcher.
    [Arguments]    ${filename}    ${kb}    ${extra}=0    ${timeout}=60s
    Open Chat With Mock
    Reset Event Cursor
    ${send_path}=    Set Variable    ${SEND_DIR}${/}${filename}
    Create Payload    ${send_path}    ${kb}    ${extra}
    Send File Via Dialog    ${send_path}
    Wait Until Keyword Succeeds    30s    3s
    ...    Chat Log Should Contain    Sending '${filename}' to
    ${req}=    Wait For Event    file.request    peer=${MOCK}
    ...    timeout=30s    fileName=${filename}
    Accept File    ${MOCK}    ${req}[fileId]
    ${done}=    Wait For Event    file.completed    peer=${MOCK}
    ...    timeout=${timeout}    fileId=${req}[fileId]
    Should Be Equal    ${done}[mode]    receive
    Wait Until Keyword Succeeds    30s    2s
    ...    Chat Log Should Contain    Completed
    ${sent}=    Get Binary File    ${send_path}
    ${recv}=    Get Binary File    ${done}[path]
    Should Be Equal    ${sent}    ${recv}
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}files-mock-${filename}.png    locator=name=ChatWindow visible=true


*** Test Cases ***
Tiny Exact KB
    [Documentation]    4KB wiring smoke: green before and after the fix.
    App Sends File Mock Receives    e2e-tiny-a.bin    4    0    30s

One Chunk Exact
    [Documentation]    65535 B = exactly one chunk (63KB+1023): single
    ...    write path, boundary logic without flush pressure.
    App Sends File Mock Receives    e2e-chunk1-a.bin    63    1023    30s

One Chunk Plus One
    [Documentation]    65537 B (64KB+1): full chunk plus a 1-byte tail write.
    App Sends File Mock Receives    e2e-chunk1p1-a.bin    64    1    30s

Two Chunks Exact
    [Documentation]    131070 B = exactly two chunks (127KB+1022): chunk
    ...    boundary with no tail.
    App Sends File Mock Receives    e2e-chunk2-a.bin    127    1022    60s

Ragged Tail
    [Documentation]    103177 B (100KB+777): multiple of neither 1024 nor
    ...    65535, ragged tail chunk.
    App Sends File Mock Receives    e2e-ragged-a.bin    100    777    60s

One Megabyte
    [Documentation]    1MB multi-chunk workhorse, fast (~16 chunks).
    App Sends File Mock Receives    e2e-small-a.bin    1024    0    60s

Regression Eight MB
    [Documentation]    The issue #3 reproducer: 8MB app -> mock must land
    ...    byte-identical. Pre-fix the mock emits file.failed with a short
    ...    file (proven 1.1MB/8MB) while the app log still flips to Completed.
    App Sends File Mock Receives    e2e-big-a.bin    8192    0    90s

LongHaul Thirty-Two MB
    [Documentation]    32MB long-haul (~500 chunks): timer drift and flake
    ...    coverage on the same code path. Costs ~1-2 min of CI.
    App Sends File Mock Receives    e2e-huge-a.bin    32768    0    180s
