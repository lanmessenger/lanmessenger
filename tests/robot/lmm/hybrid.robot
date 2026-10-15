*** Settings ***
Documentation     Hybrid E2E: the real application against one lmmd mock
...               peer. Two drivers in one suite: qtexpert+xdotool own the
...               GUI (keywords from common.resource), LmmLibrary owns the
...               mock side over the NDJSON socket. No /loopback: the app
...               runs in normal network mode and the mock is a foreign
...               peer discovered via UDP multicast.
...               Chat opening is the lmc-agent pilot (LmcAgent library,
...               todo/lmc-test-adapter.md); the rest of the GUI still rides
...               on qtexpert+xdotool until its turn comes.
...               Run with an isolated HOME (fresh chat history, no leakage
...               between runs) and a private display, e.g.:
...               HOME=/tmp/hyb-home DISPLAY=:98 robot tests/robot/lmm/
Resource          ../common.resource
Library           Process
Library           OperatingSystem
Library           LmmLibrary.py
Library           ../LmcLibrary.py    WITH NAME    LmcAgent
Suite Setup       Start Hybrid Stand
Suite Teardown    Stop Hybrid Stand
Test Setup        Focus Main Window
Test Teardown     Close All Chats


*** Variables ***
${LMMD}           ${CURDIR}/../../../build/tools/lmm/lmmd
${SOCKET}         lmm_hyb
${MOCK}           MockBob
${APP_NAME}       E2E Tester
# Normal network mode: /loopback would make the app talk to itself
# instead of discovering the mock (see messagingproc.cpp self-filter).
${APP_ARGS}       /silent
# Pilot of the in-process lmc-agent (todo/lmc-test-adapter.md): our preload
# .so rides along with qtexpert's (its launcher prepends to LD_PRELOAD
# instead of replacing it) on its own port. Overridable via -v.
${TEST_AGENT_SO}    ${CURDIR}/../../../build/tests/lmc-agent/liblmc_test_preload.so
${TEST_AGENT_PORT}    9999


*** Keywords ***
Seed App Config
    [Documentation]    Fresh lmc.ini under $HOME (caller isolates HOME per
    ...    run). Version must match the tested binary (0.0.0 locally and
    ...    in CI): a newer Version resets the config and blocks startup
    ...    with a modal warning. Same recipe as the CI Seed step.
    Create Directory    %{HOME}/.config/lmc
    ${ini}=    Catenate    SEPARATOR=\n
    ...    [General]
    ...    AutoShow=true
    ...    ${EMPTY}
    ...    [Application]
    ...    Version=0.0.0
    ...    ${EMPTY}
    ...    [User]
    ...    Name=${APP_NAME}
    Create File    %{HOME}/.config/lmc/lmc.ini    ${ini}\n

Start Hybrid Stand
    [Documentation]    Daemon + real app up, mock added, cross-visibility
    ...    confirmed on the mock side, event cursor reset for delivery.
    Seed App Config
    Create Directory    ${OUTPUT_DIR}/lmmd-home
    Start Process    ${LMMD}    --socket-name    ${SOCKET}
    ...    --home    ${OUTPUT_DIR}/lmmd-home    alias=lmmd
    Connect To Daemon    ${SOCKET}    timeout=10s
    # lmc-agent pilot: qtexpert's launcher prepends its own .so to the
    # inherited LD_PRELOAD, so both agents end up in the app process on
    # different ports (qtexpert QT_AGENT_PORT vs LMC_AGENT_PORT here).
    Set Environment Variable    LD_PRELOAD    ${TEST_AGENT_SO}
    Set Environment Variable    LMC_AGENT_PORT    ${TEST_AGENT_PORT}
    Launch LMC
    LmcAgent.Connect To Test Agent    ${TEST_AGENT_PORT}    timeout=30s
    Add Peer    ${MOCK}
    Wait For Contact    ${MOCK}    ${APP_NAME}    timeout=30s
    Reset Event Cursor

Stop Hybrid Stand
    [Documentation]    App first (its Depart still goes out), then daemon.
    ...    Everything guarded: teardown must not mask test results.
    Run Keyword And Ignore Error    Close All Chats
    Run Keyword And Ignore Error    LmcAgent.Disconnect From Test Agent
    Run Keyword And Ignore Error    Close Application
    Run Keyword And Ignore Error    Shutdown Daemon
    Run Keyword And Ignore Error    Disconnect From Daemon
    Run Keyword And Ignore Error    Terminate Process    lmmd

Open Chat With Mock
    [Documentation]    Agent-open a conversation with the mock peer
    ...    (lmc-agent pilot: Open Chat With Peer instead of the
    ...    Click tvUserList + 6x Send Key keyboard walk). Asserts the exact
    ...    title: without /loopback the suite keeps a single mock. Retried:
    ...    the mock row appears only after the app processes its announce
    ...    (seconds), and each attempt starts from a clean slate.
    Wait Until Keyword Succeeds    45s    3s    Open Chat With Mock Once

Open Chat With Mock Once
    [Documentation]    One open attempt (see Open Chat With Mock).
    ...    The xdotool search stays until Send Chat Text migrates too:
    ...    the X window id is still needed to type into the chat.
    Close All Chats
    LmcAgent.Open Chat With Peer    ${MOCK}
    Window Should Be Visible    ChatWindow
    ${title}=    Get Object Property    name=ChatWindow    windowTitle
    Should Be Equal    ${title}    ${MOCK} - Conversation
    ...    wrong peer opened (tree layout changed?)
    ${res}=    Run Process    xdotool    search    --onlyvisible    --name    Conversation
    Should Be Equal As Integers    ${res.rc}    0
    ${win}=    Evaluate    """${res.stdout}""".split()[0]
    Set Suite Variable    ${CHAT_WIN}    ${win}

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
    ${c}=    Wait For Contact    ${MOCK}    ${APP_NAME}    timeout=15s
    Wait Until Keyword Succeeds    45s    3s
    ...    Send From Mock And Expect In Log    ${c}[userId]    e2e-hyb-001

Real Sends Mock Receives
    [Documentation]    Text typed in the GUI arrives at the mock as
    ...    message.received: delivery proof without parsing the GUI.
    Open Chat With Mock
    Wait Until Keyword Succeeds    45s    5s
    ...    Send From Real And Expect Event    e2e-hyb-002
