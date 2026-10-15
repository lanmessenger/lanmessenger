*** Settings ***
Documentation     Tier-1 dialogs of LAN Messenger, single instance, no peers.
...
...               Opening goes through the in-process lmc-agent
...               (Agent Trigger Action on the menu QAction, see
...               todo/lmc-test-adapter.md): an agent trigger is queued past
...               the reply, so exec-modal dialogs opened this way keep the
...               connection alive. Driving and asserting uses the qtexpert
...               agent until its turn comes.
Resource          common.resource
Library           OperatingSystem
Library           LmcLibrary.py    WITH NAME    LmcAgent
Suite Setup       Launch LMC With Test Agent
Suite Teardown    Close LMC With Test Agent

*** Variables ***
# Our preload .so rides along with qtexpert's (its launcher prepends to
# LD_PRELOAD instead of replacing it) on its own port. Overridable via -v.
${TEST_AGENT_SO}    ${CURDIR}/../../build/tests/lmc-agent/liblmc_test_preload.so
${TEST_AGENT_PORT}    9999


*** Keywords ***
Launch LMC With Test Agent
    Set Environment Variable    LD_PRELOAD    ${TEST_AGENT_SO}
    Set Environment Variable    LMC_AGENT_PORT    ${TEST_AGENT_PORT}
    Launch LMC
    LmcAgent.Connect To Test Agent    ${TEST_AGENT_PORT}    timeout=30s

Close LMC With Test Agent
    Run Keyword And Ignore Error    LmcAgent.Disconnect From Test Agent
    Run Keyword And Ignore Error    Close Application


*** Test Cases ***
History Opens And Closes
    [Documentation]    Tools > History: modeless window, empty list
    ...    without peers. Mirrors QTest openHistory.
    Focus Main Window
    LmcAgent.Agent Trigger Action    text=History
    Window Should Be Visible    HistoryWindow
    ${title}=         Get Object Property    name=HistoryWindow    windowTitle
    Should Be Equal    ${title}    Message History
    Object Should Exist    name=tvMsgList
    Object Should Exist    name=btnClearHistory
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-history.json
    Hold For Video
    Dismiss Dialog    Message History
    Wait For Object To Disappear    name=HistoryWindow    timeout=10

File Transfers Opens And Closes
    [Documentation]    Tools > File Transfers: modeless window,
    ...    empty transfer list without peers. Mirrors QTest openFileTransfers.
    Focus Main Window
    LmcAgent.Agent Trigger Action    text="File Transfers"
    Window Should Be Visible    TransferWindow
    ${title}=         Get Object Property    name=TransferWindow    windowTitle
    Should Be Equal    ${title}    File Transfers
    Object Should Exist    name=lvTransferList
    Object Should Exist    name=btnClear
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-transfers.json
    Hold For Video
    Dismiss Dialog    File Transfers
    Window Should Be Hidden    TransferWindow

Help Opens And Closes
    [Documentation]    Help > Help: modeless window with bundled text.
    Focus Main Window
    LmcAgent.Agent Trigger Action    text=Help
    Window Should Be Visible    HelpWindow
    ${title}=         Get Object Property    name=HelpWindow    windowTitle
    Should Be Equal    ${title}    Help
    ${text}=          Get Object Property    name=txtHelp    text
    ${length}=        Evaluate    len("""${text}""")
    Should Be True    ${length} > 0    Help text is empty
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-help.json
    Hold For Video
    Dismiss Dialog    ^Help$
    Wait For Object To Disappear    name=HelpWindow    timeout=10

New Chat Room Asks For Contacts
    [Documentation]    Messenger > New Chat Room opens the room plus
    ...    a modal Select Contacts dialog (UserSelectDialog.exec()). With no
    ...    peers Cancel closes the dialog and the room window follows
    ...    (lmcCore::showChatRoomWindow closes on empty selection).
    Focus Main Window
    LmcAgent.Agent Trigger Action    text="New Chat Room"
    Window Should Be Visible    UserSelectDialog
    Object Should Exist    name=tvUserList
    Object Should Exist    name=btnOK
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-select-contacts.json
    Hold For Video
    LmcAgent.Agent Click Object    name=btnCancel
    Wait For Object To Disappear    name=UserSelectDialog    timeout=10
    Window Should Be Hidden    ChatRoomWindow
    Object Should Exist    name=MainWindow

Preferences Navigates And Cancels
    [Documentation]    Tools > Preferences: modal
    ...    dialog. Spot-check category navigation, cancel without saving.
    Focus Main Window
    LmcAgent.Agent Trigger Action    text=Preferences
    Window Should Be Visible    SettingsDialog
    Set Object Property    name=lvCategories    currentRow    1
    ${idx}=           Get Object Property    name=stackedWidget    currentIndex
    Should Be Equal As Integers    ${idx}    1
    Set Object Property    name=lvCategories    currentRow    8
    ${idx}=           Get Object Property    name=stackedWidget    currentIndex
    Should Be Equal As Integers    ${idx}    8
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-preferences.json
    Hold For Video
    Close Dialog      SettingsDialog    btnCancel

About Opens And Closes
    [Documentation]    Help > About: modal dialog
    ...    with title and tabbed texts.
    Focus Main Window
    LmcAgent.Agent Trigger Action    text=About
    Window Should Be Visible    AboutDialog
    Object Should Exist    name=lblTitle
    Object Should Exist    name=tabWidget
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-about.json
    Hold For Video
    Dismiss Dialog    About LAN Messenger
    Wait For Object To Disappear    name=AboutDialog    timeout=10
