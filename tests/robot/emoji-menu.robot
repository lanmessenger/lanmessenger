*** Settings ***
Documentation     Emoji picker popup of the chat window, single instance,
...               no peers. Opens the InstantPopup menu on the toolbar
...               button (name=btnEmoji, see chatwindow.cpp) through the
...               in-process lmc-agent and leaves evidence for manual
...               review: popup widget shot, chat context shot and whole
...               desktop capture. Shots are saved, never compared (fonts
...               and native frames float per OS). Regression cover for
...               the Windows "..." cells: an elided grid vs real glyphs
...               is self-evident on the popup shot (windows-x64 leg).
Resource          common.resource
Library           OperatingSystem
Suite Setup       Launch LMC
Suite Teardown    Close LMC
Test Teardown     Close All Chats


*** Test Cases ***
Emoji Menu Opens And Screenshots
    [Documentation]    Toolbar button pops the emoji grid; capture the
    ...    popup, the chat context and the desktop, then dismiss with
    ...    Escape without picking (nothing is inserted).
    Open Self Chat
    Window Should Be Visible    ChatWindow
    Object Should Exist    name=btnEmoji
    Click Object    name=btnEmoji
    Wait For Object    type=QMenu visible=true    timeout=10
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}emoji-menu-tree.json
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}09-emoji-menu-popup.png    locator=type=QMenu visible=true
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}09-emoji-menu-chat.png    locator=name=ChatWindow visible=true
    ${desktop}=    Capture Desktop Screenshot
    Log    DESKTOP SHOT: ${desktop}
    Send Key    Escape
    Wait For Object To Disappear    type=QMenu visible=true    timeout=10
