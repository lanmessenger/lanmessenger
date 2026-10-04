*** Settings ***
Documentation     Minimal smoke for the Robot Framework direction check.
...               Launches the real compiled binary with the qtexpert C++ agent
...               injected via LD_PRELOAD, waits for the main window and dumps
...               the UI tree. No video yet (follow-up), reports only.
Library           qtexpert    mode=preload
Suite Teardown    Close Application


*** Variables ***
# Overridden from CI via `-v APP_PATH:...` / `-v AGENT_SO:...`.
# Defaults point at the local dev-VM layout (see vm/provision docs).
${APP_PATH}       ${CURDIR}/../../lmc/release/lan-messenger
${AGENT_SO}       ${CURDIR}/../../build/qtexpert-agent/libqt_test_agent.so
${APP_ARGS}       /loopback /noconfig
${AGENT_PORT}     9988


*** Test Cases ***
Main Window Appears
    [Documentation]    App starts under the injected agent and shows MainWindow.
    ...    objectName "MainWindow" comes from lmc/src/mainwindow.ui.
    Start Application With Qt Agent    ${APP_PATH}    ${AGENT_SO}
    ...    arguments=${APP_ARGS}
    ...    port=${AGENT_PORT}
    Wait For Object    name=MainWindow    timeout=30
    Dump Object Tree    output_file=${OUTPUT_DIR}${/}ui-tree.json
