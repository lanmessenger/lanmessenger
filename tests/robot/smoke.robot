*** Settings ***
Documentation     Minimal smoke for the Robot Framework direction check.
...               Launches the agent-enabled binary through Launch LMC
...               (common.resource) and waits for the main window: one
...               in-process lmc-agent, one process, no proxy or
...               event-injection composition. The test itself only asserts
...               and dumps.
Resource          common.resource
Suite Teardown    Close LMC


*** Test Cases ***
Main Window Appears
    [Documentation]    App starts under the in-process agent and shows
    ...    MainWindow. objectName "MainWindow" comes from
    ...    lmc/src/mainwindow.ui.
    Launch LMC
    Window Should Be Visible    MainWindow
    # Existence in the object tree is not enough: hidden windows are listed
    # too (that once gave a vacuous PASS with a black screenshot). Require mapped.
    ${visible}=      Get Object Property    name=MainWindow    visible
    Should Be True    ${visible}
    ...    MainWindow exists but is not visible (AutoShow not seeded?)
    # The flag above is set synchronously by show(); mapping and first paint
    # on X are async (the QTest uses qWaitForWindowExposed for this). Give
    # the window a moment so the dump and the screenshot show real content.
    Sleep             3s
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}ui-tree.json
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}01-main-window.png
