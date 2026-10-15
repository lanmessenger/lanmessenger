*** Settings ***
Documentation     Message send/receive over the loopback self-peer, single
...               instance, no second user. With /loopback the app processes
...               its own announces, so the seeded user lists itself under
...               General and a conversation with it round-trips through the
...               real UDP+TCP stack (proven by the two-user P0 spike: typing
...               notification, send, echo). Opening and typing go through the
...               in-process lmc-agent (Open Chat With Peer for the self-peer
...               from ${SELF_PEER}, Agent Type Text into txtMessage with
...               Return — see todo/lmc-test-adapter.md). Log asserts read the
...               lmcMessageLog view via plainText, not history files (custom
...               binary format).
Resource          common.resource
Library           OperatingSystem
Library           LmcLibrary.py    WITH NAME    LmcAgent
Suite Setup       Launch LMC With Test Agent
Suite Teardown    Close LMC With Test Agent
Test Setup        Focus Main Window
Test Teardown     Close Chat


*** Variables ***
# Our preload .so rides along with qtexpert's (its launcher prepends to
# LD_PRELOAD instead of replacing it) on its own port. Overridable via -v.
${TEST_AGENT_SO}    ${CURDIR}/../../build/tests/lmc-agent/liblmc_test_preload.so
${TEST_AGENT_PORT}    9999
# Seeded user name (CI seeds E2E Tester, local runs use the dev config):
# with /loopback the app lists itself under General under this name.
${SELF_PEER}      E2E Tester


*** Keywords ***
Launch LMC With Test Agent
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

Send Chat Text Via Agent
    [Documentation]    Type text into the chat input and send with Return
    ...    (Agent Type Text posts the key, txtMessage sends on Enter via
    ...    its event filter, chatwindow.cpp). Targets the visible input
    ...    directly without stealing focus, so the keyword stays correct
    ...    with two chat windows.
    [Arguments]    ${text}
    LmcAgent.Agent Type Text    name=txtMessage visible=true    ${text}

Send Agent Message And Expect Echo
    [Documentation]    Send one marker and require its echo. The first send
    ...    on a fresh self-connection can land before the TCP handshake
    ...    completes and vanish silently; wrapping send+assert lets the
    ...    caller retry the pair with Wait Until Keyword Succeeds. Resends
    ...    are harmless: asserts only count occurrences.
    [Arguments]    ${text}
    Send Chat Text Via Agent    ${text}
    Sleep             2s
    Chat Log Should Contain    ${text}    times=2


*** Test Cases ***
Self Chat Opens
    [Documentation]    Functional presence: the self-peer row exists and
    ...    opens a conversation. The title carries the seeded user name,
    ...    which varies (CI seeds E2E Tester via ${SELF_PEER}, local runs
    ...    use the dev config) and the main-window label fills in late, so assert the shape
    ...    "<peer> - Conversation" with a non-empty peer part instead of an
    ...    exact name (P1).
    Open Self Chat Via Agent
    ${title}=        Get Object Property    name=ChatWindow    windowTitle
    Should Be True    $title.endswith(' - Conversation') and len($title) > 15    bad chat title: '${title}'
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}messages-chat.json
    Hold For Video

Message Round Trip
    [Documentation]    Typed text is sent on Return and comes back through
    ...    the loopback peer: the log holds it twice (sent line plus echo),
    ...    which is the receipt proof. Mirrors QTest sendMessage.
    Open Self Chat Via Agent
    Wait Until Keyword Succeeds    30s    3s
    ...    Send Agent Message And Expect Echo    e2e-self-001

Second Message Appends In Order
    [Documentation]    A later message lands after the earlier one in the
    ...    same conversation log.
    Open Self Chat Via Agent
    Wait Until Keyword Succeeds    30s    3s
    ...    Send Agent Message And Expect Echo    e2e-self-002a
    Wait Until Keyword Succeeds    30s    3s
    ...    Send Agent Message And Expect Echo    e2e-self-002b
    ${log}=           Read Visible Chat Log
    ${first}=         Evaluate    $log.index("e2e-self-002a")
    ${second}=        Evaluate    $log.index("e2e-self-002b")
    Should Be True    ${first} < ${second}    message order broken in chat log

Empty Message Is Ignored
    [Documentation]    Sending an empty input changes nothing
    ...    (log_sendMessage guards on isEmpty, chatwindow.cpp).
    Open Self Chat Via Agent
    ${before}=        Read Visible Chat Log
    Send Chat Text Via Agent    ${EMPTY}
    Sleep             2s
    ${after}=         Read Visible Chat Log
    Should Be Equal    ${after}    ${before}
