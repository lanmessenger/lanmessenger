*** Settings ***
Documentation     Hybrid E2E: the real application against one lmmd mock
...               peer. Two drivers in one suite: qtexpert+xdotool own the
...               GUI (keywords from common.resource), LmmLibrary owns the
...               mock side over the NDJSON socket. No /loopback: the app
...               runs in normal network mode and the mock is a foreign
...               peer discovered via UDP multicast.
...               Run with an isolated HOME (fresh chat history, no leakage
...               between runs) and a private display, e.g.:
...               HOME=/tmp/hyb-home DISPLAY=:98 robot tests/robot/lmm/
Resource          ../common.resource
Library           Process
Library           OperatingSystem
Library           LmmLibrary.py
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
    Launch LMC
    Add Peer    ${MOCK}
    Wait For Contact    ${MOCK}    ${APP_NAME}    timeout=30s
    Reset Event Cursor

Stop Hybrid Stand
    [Documentation]    App first (its Depart still goes out), then daemon.
    ...    Everything guarded: teardown must not mask test results.
    Run Keyword And Ignore Error    Close All Chats
    Run Keyword And Ignore Error    Close Application
    Run Keyword And Ignore Error    Shutdown Daemon
    Run Keyword And Ignore Error    Disconnect From Daemon
    Run Keyword And Ignore Error    Terminate Process    lmmd

Open Chat With Mock
    [Documentation]    Keyboard-open a conversation with the mock peer.
    ...    Mirrors Open Self Chat Once, but asserts the exact title:
    ...    without /loopback the app lists no self row, and the suite
    ...    keeps a single mock, so Down lands on its row. Retried: the
    ...    mock row appears only after the app processes its announce
    ...    (seconds), and each attempt starts from a clean slate.
    Wait Until Keyword Succeeds    45s    3s    Open Chat With Mock Once

Open Chat With Mock Once
    [Documentation]    One open attempt (see Open Chat With Mock).
    Close All Chats
    Focus Main Window
    Click Object      name=tvUserList
    Send Key          Home
    Send Key          Left
    Send Key          Left
    Send Key          Right
    Send Key          Down
    Send Key          Return
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
