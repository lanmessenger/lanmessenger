*** Settings ***
Documentation     Self-check of the lmmd mock daemon: two mock peers see
...               each other and deliver a message, no display needed. This
...               is NOT product coverage (mock-vs-mock only exercises the
...               network stack against itself); the real app is driven in
...               hybrid.robot. Fails fast if the daemon regresses.
Library           Process
Library           OperatingSystem
Library           LmmLibrary.py
Suite Setup       Start Mock Daemon
Suite Teardown    Stop Mock Daemon


*** Variables ***
${LMMD}           ${CURDIR}/../../../build/tools/lmm/lmmd
${SOCKET}         lmm_smoke


*** Keywords ***
Start Mock Daemon
    [Documentation]    Launch lmmd with an isolated home, connect, add two
    ...    peers and wait for cross-visibility (per PROTOCOL.md flow).
    Create Directory    ${OUTPUT_DIR}/lmmd-home
    Start Process    ${LMMD}    --socket-name    ${SOCKET}
    ...    --home    ${OUTPUT_DIR}/lmmd-home    alias=lmmd
    Connect To Daemon    ${SOCKET}    timeout=10s
    Add Peer    SmokeAlice
    Add Peer    SmokeBob
    Wait For Contact    SmokeAlice    SmokeBob    timeout=30s
    Wait For Contact    SmokeBob    SmokeAlice    timeout=30s
    Reset Event Cursor

Stop Mock Daemon
    [Documentation]    Clean shutdown: peers send Depart, no stale socket.
    Run Keyword And Ignore Error    Shutdown Daemon
    Run Keyword And Ignore Error    Disconnect From Daemon
    Run Keyword And Ignore Error    Terminate Process    lmmd


*** Test Cases ***
Mock To Mock Delivery
    [Documentation]    Alice sends by peer handle; Bob's message.received
    ...    carries the text and the sender display name.
    Send Message    SmokeAlice    SmokeBob    e2e-smoke-001
    ${ev}=    Wait For Event    message.received    peer=SmokeBob
    ...    timeout=15s    text=e2e-smoke-001
    Should Be Equal    ${ev}[fromName]    SmokeAlice

Unknown Recipient Fails Fast
    [Documentation]    Sending to nobody is a daemon error, not a silent
    ...    drop (suite continues: error is expected and caught).
    Run Keyword And Expect Error    *unknown recipient*
    ...    Send Message    SmokeAlice    NobodyHere    hi
