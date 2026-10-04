*** Settings ***
Documentation     Tier-1 dialogs of LAN Messenger, single instance, no peers.
...
...               Opening relies on real X keys (see common.resource): menu
...               shortcuts (Ctrl+H/J/N, F1) and menu mnemonics (Alt+T/P,
...               Alt+H/A). Driving and asserting uses the qtexpert agent,
...               which stays alive even while an exec-modal dialog runs as
...               long as the modal was NOT opened by an agent click.
Resource          common.resource
Suite Setup       Launch LMC
Suite Teardown    Close Application


*** Test Cases ***
History Opens And Closes
    [Documentation]    Tools > History (Ctrl+H): modeless window, empty list
    ...    without peers. Mirrors QTest openHistory.
    Focus Main Window
    Send Key          ctrl+h
    Window Should Be Visible    HistoryWindow
    ${title}=         Get Object Property    name=HistoryWindow    windowTitle
    Should Be Equal    ${title}    Message History
    Object Should Exist    name=tvMsgList
    Object Should Exist    name=btnClearHistory
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-history.json
    Dismiss Dialog    Message History
    Wait For Object To Disappear    name=HistoryWindow    timeout=10

File Transfers Opens And Closes
    [Documentation]    Tools > File Transfers (Ctrl+J): modeless window,
    ...    empty transfer list without peers. Mirrors QTest openFileTransfers.
    Focus Main Window
    Send Key          ctrl+j
    Window Should Be Visible    TransferWindow
    ${title}=         Get Object Property    name=TransferWindow    windowTitle
    Should Be Equal    ${title}    File Transfers
    Object Should Exist    name=lvTransferList
    Object Should Exist    name=btnClear
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-transfers.json
    Dismiss Dialog    File Transfers
    Window Should Be Hidden    TransferWindow

Help Opens And Closes
    [Documentation]    Help > Help (F1): modeless window with bundled text.
    Focus Main Window
    Send Key          F1
    Window Should Be Visible    HelpWindow
    ${title}=         Get Object Property    name=HelpWindow    windowTitle
    Should Be Equal    ${title}    Help
    ${text}=          Get Object Property    name=txtHelp    text
    ${length}=        Evaluate    len("""${text}""")
    Should Be True    ${length} > 0    Help text is empty
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-help.json
    Dismiss Dialog    ^Help$
    Wait For Object To Disappear    name=HelpWindow    timeout=10

New Chat Room Asks For Contacts
    [Documentation]    Messenger > New Chat Room (Ctrl+N) opens the room plus
    ...    a modal Select Contacts dialog (UserSelectDialog.exec()). With no
    ...    peers Cancel closes the dialog and the room window follows
    ...    (lmcCore::showChatRoomWindow closes on empty selection).
    Focus Main Window
    Send Key          ctrl+n
    Window Should Be Visible    UserSelectDialog
    Object Should Exist    name=tvUserList
    Object Should Exist    name=btnOK
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-select-contacts.json
    Click Object      name=btnCancel
    Wait For Object To Disappear    name=UserSelectDialog    timeout=10
    Window Should Be Hidden    ChatRoomWindow
    Object Should Exist    name=MainWindow

Preferences Navigates And Cancels
    [Documentation]    Tools > Preferences via mnemonics (Alt+T, P): modal
    ...    dialog. Spot-check category navigation, cancel without saving.
    ...    The Ctrl+, standard shortcut does not fire under Xvfb, hence the
    ...    mnemonic path (proven by spike).
    Focus Main Window
    Send Key          alt+t
    Sleep             1s
    Send Key          p
    Window Should Be Visible    SettingsDialog
    Set Object Property    name=lvCategories    currentRow    1
    ${idx}=           Get Object Property    name=stackedWidget    currentIndex
    Should Be Equal As Integers    ${idx}    1
    Set Object Property    name=lvCategories    currentRow    8
    ${idx}=           Get Object Property    name=stackedWidget    currentIndex
    Should Be Equal As Integers    ${idx}    8
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-preferences.json
    Close Dialog      SettingsDialog    btnCancel

About Opens And Closes
    [Documentation]    Help > About via mnemonics (Alt+H, A): modal dialog
    ...    with title and tabbed texts.
    Focus Main Window
    Send Key          alt+h
    Sleep             1s
    Send Key          a
    Window Should Be Visible    AboutDialog
    Object Should Exist    name=lblTitle
    Object Should Exist    name=tabWidget
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}dialogs-about.json
    Dismiss Dialog    About LAN Messenger
    Wait For Object To Disappear    name=AboutDialog    timeout=10
