*** Settings ***
Documentation     Message send/receive over the loopback self-peer, single
...               instance, no second user. With /loopback the app processes
...               its own announces, so the seeded user lists itself under
...               General and a conversation with it round-trips through the
...               real UDP+TCP stack (proven by the two-user P0 spike: typing
...               notification, send, echo). Opening is keyboard-only (see
...               Open Self Chat): an agent click that opens an exec-modal
...               dialog blocks the agent connection, and toolbar buttons
...               cannot be triggered any other way. Log asserts read the
...               lmcMessageLog view via plainText, not history files (custom
...               binary format).
Resource          common.resource
Suite Setup       Launch LMC
Suite Teardown    Close Application
Test Setup        Focus Main Window
Test Teardown     Close Chat


*** Test Cases ***
Self Chat Opens
    [Documentation]    Functional presence: the self-peer row exists and
    ...    opens a conversation. The title carries the seeded user name,
    ...    which varies (CI seeds E2E Tester, local runs use the dev config)
    ...    and the main-window label fills in late, so assert the shape
    ...    "<peer> - Conversation" with a non-empty peer part instead of an
    ...    exact name (P1).
    Open Self Chat
    ${title}=        Get Object Property    name=ChatWindow    windowTitle
    Should Be True    $title.endswith(' - Conversation') and len($title) > 15    bad chat title: '${title}'
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}messages-chat.json
    Hold For Video

Message Round Trip
    [Documentation]    Typed text is sent on Return and comes back through
    ...    the loopback peer: the log holds it twice (sent line plus echo),
    ...    which is the receipt proof. Mirrors QTest sendMessage.
    Open Self Chat
    Wait Until Keyword Succeeds    30s    3s
    ...    Send Chat Message And Expect Echo    e2e-self-001

Second Message Appends In Order
    [Documentation]    A later message lands after the earlier one in the
    ...    same conversation log.
    Open Self Chat
    Wait Until Keyword Succeeds    30s    3s
    ...    Send Chat Message And Expect Echo    e2e-self-002a
    Wait Until Keyword Succeeds    30s    3s
    ...    Send Chat Message And Expect Echo    e2e-self-002b
    ${log}=           Read Visible Chat Log
    ${first}=         Evaluate    $log.index("e2e-self-002a")
    ${second}=        Evaluate    $log.index("e2e-self-002b")
    Should Be True    ${first} < ${second}    message order broken in chat log

Empty Message Is Ignored
    [Documentation]    Sending an empty input changes nothing
    ...    (log_sendMessage guards on isEmpty, chatwindow.cpp).
    Open Self Chat
    ${before}=        Read Visible Chat Log
    Send Chat Text    ${EMPTY}
    Sleep             2s
    ${after}=         Read Visible Chat Log
    Should Be Equal    ${after}    ${before}
