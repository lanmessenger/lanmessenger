*** Settings ***
Documentation     Desktop screenshots E2E: the real application against lmmd
...               mock peers — the fixed scene for the site hero shots.
...               One instance (MainWindow + ChatWindow), no second client:
...               the single-instance socket path is covered by
...               second_instance.robot instead.
...               Contact list follows site/src/data/mockup-contacts.ts
...               (all 8 people, same order, names with spaces —
...               lmmd peer.add accepts single internal spaces;
...               away/busy mirror the mockup
...               where the row stays visible, see Variables).
...               The dialog is 5 replicas without emoji and without
...               v{version}. Windows are placed
...               deterministically via the agent (set_geometry: no WM on
...               Linux CI, bare Xvfb): v2 layout for 1280x1024 spreads Main
...               (64,96 360x620) and Chat (560,240 600x640) diagonally with
...               desktop visible around and between them, no overlap. The dialog markers
...               (LINE_*) are suite-unique: CI runs the whole tests/robot/
...               with one shared HOME, so no other suite may reuse these
...               texts (stale chat history would cross-match). The desktop capture
...               lands in OUTPUT_DIR (robot-results-<leg>): Linux raw xwd
...               (no conversion in v1), Windows PNG via PowerShell,
...               macOS screencapture with widget-shot fallback. Shots are
...               saved, never compared (clocks, font smoothing and native
...               decorations float per OS). No autocommit to site/ in v1 —
...               updating site/public/images/lmc_1225_* stays manual.
...               Two drivers in one suite, one per side: LmcLibrary
...               (via common.resource) owns the GUI with the in-process
...               lmc-agent, LmmLibrary (via mockstand.resource) owns the
...               mock side over its NDJSON socket. No /loopback: the app
...               runs in normal network mode and the mocks are foreign
...               peers discovered via UDP multicast.
...               Run with an isolated HOME (fresh chat history, no leakage
...               between runs), e.g.:
...               HOME=/tmp/shots-home DISPLAY=:99 robot tests/robot/screenshots.robot
Resource          common.resource
Resource          mockstand.resource
Suite Setup       Start Screenshots Stand
Suite Teardown    Stop Screenshots Stand
Test Teardown     Close All Chats


*** Variables ***
${LMM_PORT}       17773
# Free name choice: Seed App Config + /config= (see Launch LMC) imports
# User/Name on every leg, so the suite is not bound to the CI seed name.
# Override locally without editing: robot -v APP_NAME:Zoe ...
${APP_NAME}       Alice
# Normal network mode: /loopback would make the app talk to itself
# instead of discovering the mocks (see messagingproc.cpp self-filter).
${APP_ARGS}       /silent
# Contact list: the 8 people of site/src/data/mockup-contacts.ts, in the
# same order. Names carry spaces (lmmd peer.add allows single internal
# spaces — tools/lmm/daemon/rpc.h validName); the announced name shown in
# tvUserList IS the handle, so the list matches the mockup. Avatars stay
# default (mocks have none).
${PEER_BOB}       Bob
${PEER_JAMES}     James Smith
${PEER_MARIA}     Maria Garcia
${PEER_ROBERT}    Robert Johnson
${PEER_LINDA}     Linda Brown
${PEER_MICHAEL}   Michael Davis
${PEER_SARAH}     Sarah Miller
${PEER_DAVID}     David Wilson
# Statuses mirror the mockup where the row stays visible: Michael away,
# Sarah busy (mockup statusIcons). David is offline in the mockup, but an
# offline-type status ("gone") suppresses MT_Announce to the app layer
# (messaging.cpp addUser) — the row would vanish — so David stays online.
# v1 dialog: 5 replicas, no emoji, no v{version} (motifs from
# site/src/data/mockup-dialog.ts, version/emoji lines dropped).
${LINE_1}         Ready for release?
${LINE_2}         One last test. Disconnect the internet.
${LINE_3}         Done. Still chatting.
${LINE_4}         File transfer works too.
${LINE_5}         Perfect. Ship it!
# Deterministic placement under 1280x1024 (bare Xvfb, no WM):
# set_geometry + raise + activateWindow via Place Window. v2 spreads
# the windows diagonally — Main upper-left, Chat lower-right — with
# desktop visible on every side and a gap between them (no overlap).
${MAIN_X}         64
${MAIN_Y}         96
${MAIN_W}         360
${MAIN_H}         620
${CHAT_X}         560
${CHAT_Y}         240
${CHAT_W}         600
${CHAT_H}         640


*** Keywords ***
Start Screenshots Stand
    [Documentation]    Daemon + real app up, 8 mocks added, cross-visibility
    ...    confirmed on the mock side, event cursor reset for delivery.
    ${ini}=    Seed App Config    ${APP_NAME}
    Start Mock Stand    ${LMM_PORT}    ${TEMPDIR}${/}e2e-lmmd-${LMM_PORT}
    Launch LMC    ${ini}
    Add Peer    ${PEER_BOB}
    Add Peer    ${PEER_JAMES}
    Add Peer    ${PEER_MARIA}
    Add Peer    ${PEER_ROBERT}
    Add Peer    ${PEER_LINDA}
    Add Peer    ${PEER_MICHAEL}    away
    Add Peer    ${PEER_SARAH}    busy
    Add Peer    ${PEER_DAVID}
    Wait For Contact Ready    ${PEER_BOB}    ${APP_NAME}    timeout=30s
    Wait For Contact Ready    ${PEER_JAMES}    ${APP_NAME}    timeout=30s
    Wait For Contact Ready    ${PEER_MARIA}    ${APP_NAME}    timeout=30s
    Wait For Contact Ready    ${PEER_ROBERT}    ${APP_NAME}    timeout=30s
    Wait For Contact Ready    ${PEER_LINDA}    ${APP_NAME}    timeout=30s
    Wait For Contact Ready    ${PEER_MICHAEL}    ${APP_NAME}    timeout=30s
    Wait For Contact Ready    ${PEER_SARAH}    ${APP_NAME}    timeout=30s
    Wait For Contact Ready    ${PEER_DAVID}    ${APP_NAME}    timeout=30s
    Reset Event Cursor

Stop Screenshots Stand
    [Documentation]    App first (its Depart still goes out), then daemon.
    ...    Everything guarded: teardown must not mask test results.
    Run Keyword And Ignore Error    Close LMC
    Run Keyword And Ignore Error    Stop Mock Stand

Open Chat With Bob Once
    [Documentation]    One open attempt (see chat.robot): resolve the Bob
    ...    contact and fire the real chatStarting path.
    Close All Chats
    Open Chat With Peer    ${PEER_BOB}
    Window Should Be Visible    ChatWindow

Open Chat With Bob
    [Documentation]    Retried open: the mock row appears only after the app
    ...    processes its announce (seconds), and each attempt starts from
    ...    a clean slate.
    Wait Until Keyword Succeeds    45s    3s    Open Chat With Bob Once
    ${title}=    Get Object Property    name=ChatWindow    windowTitle
    Should Be Equal    ${title}    ${PEER_BOB} - Conversation
    ...    wrong peer opened (tree layout changed?)

Send From Mock And Expect In Log
    [Documentation]    One mock send plus the GUI-log assert, as a retry
    ...    unit: the first send on a fresh connection can vanish silently
    ...    (P1); resends only append, the count assert stays valid.
    [Arguments]    ${to}    ${text}
    Send Message    ${PEER_BOB}    ${to}    ${text}
    Sleep    2s
    Chat Log Should Contain    ${text}    times=1

Send From Real And Expect Event
    [Documentation]    One GUI send plus the mock-side receipt assert.
    [Arguments]    ${text}
    Send Chat Text    ${text}
    ${ev}=    Wait For Event    message.received    peer=${PEER_BOB}
    ...    timeout=12s    text=${text}
    Should Be Equal    ${ev}[fromName]    ${APP_NAME}


*** Test Cases ***
Capture Desktop Scene
    [Documentation]    Fixed v1 scene: MainWindow + ChatWindow (Bob) placed
    ...    deterministically, 5-replica dialog both ways, whole-desktop
    ...    capture plus widget-shot control duplicates. Never compared —
    ...    artifacts only (robot-results-<leg>).
    Open Chat With Bob
    Place Window    name=MainWindow    ${MAIN_X}    ${MAIN_Y}    ${MAIN_W}    ${MAIN_H}
    Place Window    name=ChatWindow    ${CHAT_X}    ${CHAT_Y}    ${CHAT_W}    ${CHAT_H}
    Sleep    2s
    ${c}=    Wait For Contact Ready    ${PEER_BOB}    ${APP_NAME}    timeout=15s
    Wait Until Keyword Succeeds    45s    3s
    ...    Send From Real And Expect Event    ${LINE_1}
    Wait Until Keyword Succeeds    45s    3s
    ...    Send From Mock And Expect In Log    ${c}[userId]    ${LINE_2}
    Wait Until Keyword Succeeds    45s    5s
    ...    Send From Real And Expect Event    ${LINE_3}
    Wait Until Keyword Succeeds    45s    5s
    ...    Send From Real And Expect Event    ${LINE_4}
    Wait Until Keyword Succeeds    45s    3s
    ...    Send From Mock And Expect In Log    ${c}[userId]    ${LINE_5}
    Sleep    1s
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}screenshots-ui-tree.json
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}screenshots-widget-main.png    locator=name=MainWindow visible=true
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}screenshots-widget-chat.png    locator=name=ChatWindow visible=true
    ${desktop}=    Capture Desktop Screenshot
    Log    DESKTOP SHOT: ${desktop}
