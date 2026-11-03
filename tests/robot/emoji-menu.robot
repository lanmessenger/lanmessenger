*** Settings ***
Documentation     Emoji picker popup of the chat window, single instance,
...               no peers. Opens the InstantPopup menu on the toolbar
...               button (name=btnEmoji, see chatwindow.cpp) through the
...               in-process lmc-agent and leaves evidence for manual
...               review: popup widget shot, chat context shot and whole
...               desktop capture, plus hover-highlight and keyboard-focus
...               states of the grid. Shots are saved, never compared (fonts
...               and native frames float per OS). Regression cover for
...               the Windows "..." cells: an elided grid vs real glyphs
...               is self-evident on the popup shot (windows-x64 leg); the
...               hover/keyboard shots prove the Highlight delegate path
...               (State_MouseOver / Selected+HasFocus, see emojipicker.cpp).
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

Emoji Menu Hover Highlights Cell
    [Documentation]    Synthetic mouse hover over cell (0,2) via the
    ...    agent hover command (MouseMove+HoverMove on the picker
    ...    viewport, fire-and-post); the delegate Highlight on
    ...    State_MouseOver must be self-evident on the popup shot.
    ...    Dismiss with Escape without picking.
    Open Self Chat
    Window Should Be Visible    ChatWindow
    Object Should Exist    name=btnEmoji
    Click Object    name=btnEmoji
    Wait For Object    type=QMenu visible=true    timeout=10
    Wait For Object    type=EmojiPicker visible=true    timeout=10
    Hover Object    type=EmojiPicker visible=true    0    2
    Sleep    1s
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}09-emoji-menu-hover-popup.png    locator=type=QMenu visible=true
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}09-emoji-menu-hover-chat.png    locator=name=ChatWindow visible=true
    Send Key    Escape
    Wait For Object To Disappear    type=QMenu visible=true    timeout=10

Emoji Menu Keyboard Moves Current Cell
    [Documentation]    Focus the grid, then walk (Right, Down) with arrows
    ...    posted straight to the EmojiPicker; the Highlight on
    ...    Selected/HasFocus must be self-evident on the popup shot.
    ...    Dismiss with Escape.
    Open Self Chat
    Window Should Be Visible    ChatWindow
    Object Should Exist    name=btnEmoji
    Click Object    name=btnEmoji
    Wait For Object    type=QMenu visible=true    timeout=10
    Wait For Object    type=EmojiPicker visible=true    timeout=10
    Wait Until Keyword Succeeds    10s    0.5s    Focus Emoji Grid    type=EmojiPicker visible=true
    Send Key    Right    type=EmojiPicker visible=true
    Sleep    0.5s
    Send Key    Down    type=EmojiPicker visible=true
    Sleep    0.5s
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}09-emoji-menu-keyboard-popup.png    locator=type=QMenu visible=true
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}09-emoji-menu-keyboard-chat.png    locator=name=ChatWindow visible=true
    Send Key    Escape
    Wait For Object To Disappear    type=QMenu visible=true    timeout=10


*** Keywords ***
Focus Emoji Grid
    [Documentation]    Focus the picker and require ``hasFocus``: the agent
    ...    fails when the widget does not accept focus (popup not active
    ...    yet under Xvfb), so retry until it does — otherwise the arrow
    ...    keys still land (explicit locator posts directly) but the
    ...    Selected/HasFocus Highlight never paints and the screenshot
    ...    silently proves nothing.
    [Arguments]    ${locator}
    ${focus}=    Focus Object    ${locator}
    Should Be True    ${focus}[hasFocus]    Emoji grid did not accept focus: ${focus}
