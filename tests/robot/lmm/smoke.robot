*** Settings ***
Documentation     Self-check of the lmmd mock daemon: two mock peers see
...               each other and deliver a message, no display needed. This
...               is NOT product coverage (mock-vs-mock only exercises the
...               network stack against itself); the real app is driven in
...               the top-level suites. Fails fast if the daemon regresses.
Resource          ../mockstand.resource
Suite Setup       Start Smoke Stand
Suite Teardown    Stop Mock Stand


*** Variables ***
${LMM_PORT}       17773


*** Keywords ***
Start Smoke Stand
    [Documentation]    Daemon up, two peers cross-visible, event cursor
    ...    reset for delivery (per tools/lmm/PROTOCOL.md flow).
    Start Mock Stand    ${LMM_PORT}    ${OUTPUT_DIR}/lmmd-home
    Add Peer    SmokeAlice
    Add Peer    SmokeBob
    Wait For Contact Ready    SmokeAlice    SmokeBob    timeout=30s
    Wait For Contact Ready    SmokeBob    SmokeAlice    timeout=30s
    Reset Event Cursor


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
