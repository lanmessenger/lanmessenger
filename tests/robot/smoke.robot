*** Settings ***
Documentation     Minimal smoke for the Robot Framework direction check.
...               Launches the real compiled binary with the qtexpert C++ agent
...               injected via LD_PRELOAD, waits for the main window and dumps
...               the UI tree. Video is captured outside the test (ffmpeg in
...               CI), the test itself only asserts and dumps.
Library           qtexpert    mode=preload
Suite Teardown    Close Application


*** Variables ***
# Overridden from CI via `-v APP_PATH:...` / `-v AGENT_SO:...`.
# Defaults point at the local dev-VM layout (see vm/provision docs).
${APP_PATH}       ${CURDIR}/../../lmc/release/lan-messenger
${AGENT_SO}       ${CURDIR}/../../build/qtexpert-agent/libqt_test_agent.so
# The main window is shown at startup only with AutoShow=true in
# $HOME/.config/lmc/lmc.ini (lmcMainWindow::start checks it, default false).
# CI pre-seeds that file in the Seed step; never pass /noconfig here, it
# deletes the settings file. Mirrors tests/e2e/tst_lmc_gui.cpp.
${APP_ARGS}       /silent /loopback
${AGENT_PORT}     9988


*** Test Cases ***
Main Window Appears
    [Documentation]    App starts under the injected agent and shows MainWindow.
    ...    objectName "MainWindow" comes from lmc/src/mainwindow.ui.
    Start Application With Qt Agent    ${APP_PATH}    ${AGENT_SO}
    ...    arguments=${APP_ARGS}
    ...    port=${AGENT_PORT}
    Wait For Object    name=MainWindow    timeout=30
    # Existence in the object tree is not enough: hidden windows are listed
    # too (that gave us a vacuous PASS with a black video). Require mapped.
    ${visible}=      Get Object Property    name=MainWindow    visible
    Should Be True    ${visible}    MainWindow exists but is not visible (AutoShow not seeded?)
    # The flag above is set synchronously by show(); mapping and first paint
    # on X are async (the QTest uses qWaitForWindowExposed for this). Give
    # the window a moment so the dump and the CI video show real content.
    Sleep             3s
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}ui-tree.json
