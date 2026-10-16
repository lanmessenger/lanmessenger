*** Settings ***
Documentation     Tier-1 dialogs of LAN Messenger, single instance, no peers.
...
...               Opening goes through the in-process lmc-agent
...               (Trigger Action on the menu QAction, see
...               todo/lmc-test-adapter.md): an agent trigger is queued past
...               the reply, so exec-modal dialogs opened this way keep the
...               connection alive. Driving, asserting and closing all run
...               on the same agent (see common.resource).
Resource          common.resource
Library           OperatingSystem
Suite Setup       Launch LMC
Suite Teardown    Close LMC
Test Teardown     Log App Liveness On Failure


*** Test Cases ***
History Opens And Closes
    [Documentation]    Tools > History: modeless window, empty list
    ...    without peers. Mirrors QTest openHistory.
    Trigger Action    text=History
    Window Should Be Visible    HistoryWindow
    ${title}=         Get Object Property    name=HistoryWindow    windowTitle
    Should Be Equal    ${title}    Message History
    Object Should Exist    name=tvMsgList
    Object Should Exist    name=btnClearHistory
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-history.json
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}03-message-history.png    locator=name=HistoryWindow
    Hold For Video
    Close Window    name=HistoryWindow
    Wait For Object To Disappear    name=HistoryWindow    timeout=10

File Transfers Opens And Closes
    [Documentation]    Tools > File Transfers: modeless window,
    ...    empty transfer list without peers. Mirrors QTest openFileTransfers.
    Trigger Action    text="File Transfers"
    Window Should Be Visible    TransferWindow
    ${title}=         Get Object Property    name=TransferWindow    windowTitle
    Should Be Equal    ${title}    File Transfers
    Object Should Exist    name=lvTransferList
    Object Should Exist    name=btnClear
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-transfers.json
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}02-file-transfers.png    locator=name=TransferWindow
    Hold For Video
    Close Window    name=TransferWindow
    Window Should Be Hidden    TransferWindow

Help Opens And Closes
    [Documentation]    Help > Help: modeless window with bundled text.
    Trigger Action    text=Help
    Window Should Be Visible    HelpWindow
    ${title}=         Get Object Property    name=HelpWindow    windowTitle
    Should Be Equal    ${title}    Help
    ${text}=          Get Object Property    name=txtHelp    text
    ${length}=        Evaluate    len("""${text}""")
    Should Be True    ${length} > 0    Help text is empty
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-help.json
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}05-help.png    locator=name=HelpWindow
    Hold For Video
    Close Window    name=HelpWindow
    Wait For Object To Disappear    name=HelpWindow    timeout=10

New Chat Room Asks For Contacts
    [Documentation]    Messenger > New Chat Room opens the room plus
    ...    a modal Select Contacts dialog (UserSelectDialog.exec()). With no
    ...    peers Cancel closes the dialog and the room window follows
    ...    (lmcCore::showChatRoomWindow closes on empty selection).
    Trigger Action    text="New Chat Room"
    Window Should Be Visible    UserSelectDialog
    Object Should Exist    name=tvUserList
    Object Should Exist    name=btnOK
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-select-contacts.json
    Hold For Video
    Click Object    name=btnCancel
    Wait For Object To Disappear    name=UserSelectDialog    timeout=10
    Window Should Be Hidden    ChatRoomWindow
    Object Should Exist    name=MainWindow

Preferences Navigates And Cancels
    [Documentation]    Tools > Preferences: modal
    ...    dialog. Spot-check category navigation, cancel without saving.
    Trigger Action    text=Preferences
    Window Should Be Visible    SettingsDialog
    Set Object Property    name=lvCategories    currentRow    1
    ${idx}=           Get Object Property    name=stackedWidget    currentIndex
    Should Be Equal As Integers    ${idx}    1
    Set Object Property    name=lvCategories    currentRow    8
    ${idx}=           Get Object Property    name=stackedWidget    currentIndex
    Should Be Equal As Integers    ${idx}    8
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-preferences.json
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}06-preferences.png    locator=name=SettingsDialog
    Hold For Video
    Close Dialog      SettingsDialog    btnCancel

About Opens And Closes
    [Documentation]    Help > About: modal dialog
    ...    with title and tabbed texts.
    Trigger Action    text=About
    Window Should Be Visible    AboutDialog
    Object Should Exist    name=lblTitle
    Object Should Exist    name=tabWidget
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-about.json
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}04-about-dialog.png    locator=name=AboutDialog
    Hold For Video
    Close Window    name=AboutDialog
    Wait For Object To Disappear    name=AboutDialog    timeout=10
