*** Settings ***
Documentation     Fail-fast: an unexpected worker death exits the daemon
...               with a non-zero code instead of masking the crash.
...               Mock-only, no display needed. The daemon is expected to
...               be dead after the test, so there is no graceful teardown.
Library           Process
Resource          ../mockstand.resource
Suite Setup       Start Crash Stand
Suite Teardown    Run Keyword And Ignore Error    Terminate Process    lmmd


*** Variables ***
${SOCKET}         lmm_crash
${PEER_HOME}      ${OUTPUT_DIR}/lmmd-home-crash


*** Keywords ***
Start Crash Stand
    [Documentation]    Daemon + two peers; the mesh itself is not asserted
    ...    here, only that both workers are up before the kill.
    Start Mock Stand    ${SOCKET}    ${PEER_HOME}
    Add Peer    CrashAlice
    Add Peer    CrashBob

Worker Cmdline
    [Documentation]    Full command line of a pid (to verify the kill target).
    [Arguments]    ${pid}
    ${ps}=    Run Process    ps    --no-headers    -p    ${pid}    -o    args
    ...    stdout=${OUTPUT_DIR}/ps-${pid}.txt
    Should Be Equal As Integers    ${ps.rc}    0
    RETURN    ${ps.stdout}


*** Test Cases ***
Worker Crash Exits Daemon Non-Zero
    [Documentation]    SIGKILL one worker; the supervisor must log the peer
    ...    name and exit non-zero (fail-fast, no silent masking).
    ${pg}=    Run Process    pgrep    -f    lmmd --worker --peer CrashBob
    Should Be Equal As Integers    ${pg.rc}    0
    ${pid}=    Evaluate    """${pg.stdout}""".split()[0]
    ${cmd}=    Worker Cmdline    ${pid}
    Should Contain    ${cmd}    --worker --peer CrashBob
    Run Process    kill    -9    ${pid}
    ${res}=    Wait For Process    lmmd    timeout=30s
    Should Not Be Equal As Integers    ${res.rc}    0
