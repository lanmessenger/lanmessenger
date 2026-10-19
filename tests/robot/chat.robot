*** Settings ***
Documentation     Chat E2E: the real application against one lmmd mock
...               peer — the primary chat scenario (compare chat.loopback
...               for the same feature over the loopback self-peer).
...               Two drivers in one suite, one per side: LmcLibrary
...               (via common.resource) owns the GUI with the in-process
...               lmc-agent, LmmLibrary (via mockstand.resource) owns the
...               mock side over its NDJSON socket. No /loopback: the app
...               runs in normal network mode and the mock is a foreign
...               peer discovered via UDP multicast.
...               Run with an isolated HOME (fresh chat history, no leakage
...               between runs), e.g.:
...               HOME=/tmp/chat-home DISPLAY=:99 robot tests/robot/chat.robot
Resource          common.resource
Resource          mockstand.resource
Suite Setup       Start Chat Stand
Suite Teardown    Stop Chat Stand
Test Teardown     Close All Chats


*** Variables ***
${LMM_PORT}       17771
${MOCK}           MockBob
${APP_NAME}       E2E Tester
# Normal network mode: /loopback would make the app talk to itself
# instead of discovering the mock (see messagingproc.cpp self-filter).
${APP_ARGS}       /silent


*** Keywords ***
Start Chat Stand
    [Documentation]    Daemon + real app up, mock added, cross-visibility
    ...    confirmed on the mock side, event cursor reset for delivery.
    Seed App Config    ${APP_NAME}
    Start Mock Stand    ${LMM_PORT}    ${OUTPUT_DIR}/lmmd-home
    Launch LMC
    Add Peer    ${MOCK}
    Wait For Contact Ready    ${MOCK}    ${APP_NAME}    timeout=30s
    Reset Event Cursor

Stop Chat Stand
    [Documentation]    App first (its Depart still goes out), then daemon.
    ...    Everything guarded: teardown must not mask test results.
    Run Keyword And Ignore Error    Close LMC
    Run Keyword And Ignore Error    Stop Mock Stand

Open Chat With Mock
    [Documentation]    Agent-open a conversation with the mock peer (Open
    ...    Chat With Peer resolves the contact in the main-window tree and
    ...    fires the same chatStarting path as keyboard Return). Asserts the
    ...    exact title: without /loopback the suite keeps a single mock.
    ...    Retried: the mock row appears only after the app processes its
    ...    announce (seconds), and each attempt starts from a clean slate.
    Wait Until Keyword Succeeds    45s    3s    Open Chat With Mock Once

Open Chat With Mock Once
    [Documentation]    One open attempt (see Open Chat With Mock).
    Close All Chats
    Open Chat With Peer    ${MOCK}
    Window Should Be Visible    ChatWindow
    ${title}=    Get Object Property    name=ChatWindow    windowTitle
    Should Be Equal    ${title}    ${MOCK} - Conversation
    ...    wrong peer opened (tree layout changed?)

Send From Mock And Expect In Log
    [Documentation]    One mock send plus the GUI-log assert, as a retry
    ...    unit: the first send on a fresh connection can vanish silently
    ...    (P1); resends only append, the count assert stays valid.
    [Arguments]    ${to}    ${text}
    Send Message    ${MOCK}    ${to}    ${text}
    Sleep    2s
    Chat Log Should Contain    ${text}    times=1

Send From Real And Expect Event
    [Documentation]    One GUI send plus the mock-side receipt assert.
    [Arguments]    ${text}
    Send Chat Text    ${text}
    ${ev}=    Wait For Event    message.received    peer=${MOCK}
    ...    timeout=12s    text=${text}
    Should Be Equal    ${ev}[fromName]    ${APP_NAME}


*** Test Cases ***
Mock Sends Real Receives
    [Documentation]    The mock sends to the app's userId; the text lands
    ...    in the open GUI conversation.
    Open Chat With Mock
    ${c}=    Wait For Contact Ready    ${MOCK}    ${APP_NAME}    timeout=15s
    Wait Until Keyword Succeeds    45s    3s
    ...    Send From Mock And Expect In Log    ${c}[userId]    e2e-hyb-001
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}07-chat-mock.png    locator=name=ChatWindow visible=true

Real Sends Mock Receives
    [Documentation]    Text typed in the GUI arrives at the mock as
    ...    message.received: delivery proof without parsing the GUI.
    Open Chat With Mock
    Wait Until Keyword Succeeds    45s    5s
    ...    Send From Real And Expect Event    e2e-hyb-002
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}08-chat-mock-with-text.png    locator=name=ChatWindow visible=true
