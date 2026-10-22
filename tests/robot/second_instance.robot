*** Settings ***
Documentation     Single-instance control: a second launch of the same binary
...               delivers "raise" over the single-instance socket and exits,
...               and the first instance restores its hidden main window.
...               This covers the IPC path (a real second process exists);
...               the macOS Dock/Finder reopen creates no second process and
...               is NOT covered here — green here plus a red manual reopen
...               isolates the bug to reopen delivery, not to restore().
Resource          common.resource
Library           OperatingSystem
Suite Setup       Launch LMC
Suite Teardown    Close Second Instance Suite
Test Teardown     Log App Liveness On Failure


*** Test Cases ***
Second Launch Raises Hidden Main Window
    [Documentation]    Hide the main window (same path as the close button:
    ...    closeEvent ignores and minimizes to hidden), launch the same
    ...    binary again, require the second process to exit 0
    ...    (SecondDelivered) and the first instance's MainWindow to become
    ...    visible again.
    Close Window    name=MainWindow
    Window Should Be Hidden    MainWindow
    Launch Second Instance
    Wait Until Keyword Succeeds    10s    500ms
    ...    Window Should Be Visible    MainWindow
    Grab Screenshot    output_file=${OUTPUT_DIR}${/}09-second-instance-raised.png    locator=name=MainWindow visible=true


*** Keywords ***
Launch Second Instance
    [Documentation]    Start the same binary with the same args as Launch LMC
    ...    (plus /trace like the first instance, never /noconfig — it would
    ...    delete the seeded settings). The second process must deliver
    ...    "raise" to the first instance and exit 0 without ever starting
    ...    its own agent (apprunner returns before the pre-exec hook).
    @{app_args}=    Split Command Line    ${APP_ARGS}
    @{app_args}=    Evaluate
    ...    $app_args + ([] if any(a.lower() == '/trace' for a in $app_args) else ['/trace'])
    Start Process    ${APP_PATH}    @{app_args}    alias=second
    ...    stdout=${OUTPUT_DIR}${/}lmc-app-second-instance.log    stderr=STDOUT
    ${res}=    Wait For Process    second    timeout=15s
    Should Be Equal As Integers    ${res.rc}    0
    ...    Second instance should deliver raise and exit 0 (SecondDelivered)

Close Second Instance Suite
    [Documentation]    Belt-and-braces: the second instance must already be
    ...    gone (it exits on delivery); never let a stray alias=second
    ...    survive into the next suite, then the standard Close LMC (a stale
    ...    single-instance socket would silently absorb the next launch).
    Run Keyword And Ignore Error    Terminate Process    second
    Close LMC
