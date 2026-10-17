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
${LMM_PORT}       17774
${PEER_HOME}      ${OUTPUT_DIR}/lmmd-home-crash


*** Keywords ***
Start Crash Stand
    [Documentation]    Daemon + two peers; the mesh itself is not asserted
    ...    here, only that both workers are up before the kill.
    Start Mock Stand    ${LMM_PORT}    ${PEER_HOME}
    Add Peer    CrashAlice
    Add Peer    CrashBob

Worker Pid
    [Documentation]    PID of the worker process serving a peer handle,
    ...    reported by the daemon itself (worker.status `pid`): no
    ...    process-table parsing, so no platform shell quoting to get
    ...    wrong (an RF `name=value` token inside a PowerShell one-liner
    ...    once parsed as a Run Process named argument).
    [Arguments]    ${peer}
    ${st}=    Worker Status    ${peer}
    ${pid}=    Set Variable    ${st}[pid]
    Should Match Regexp    ${pid}    ^[0-9]+$
    RETURN    ${pid}

Worker Process Should Be An Lmmd Worker
    [Documentation]    The reported pid is a live lmmd process: the
    ...    kill-target check. Unix `ps -p PID -o args` (portable, BSD-safe);
    ...    Windows `tasklist /FI "PID eq N"` — `eq`, never `=` (see above).
    [Arguments]    ${pid}
    ${sys}=    Evaluate    platform.system()    modules=platform
    IF    $sys == 'Windows'
        ${ps}=    Run Process    tasklist    /FI    PID eq ${pid}
        ...    stdout=${OUTPUT_DIR}/tasklist-${pid}.txt
    ELSE
        ${ps}=    Run Process    ps    -p    ${pid}    -o    args
        ...    stdout=${OUTPUT_DIR}/ps-${pid}.txt
    END
    Should Be Equal As Integers    ${ps.rc}    0
    Should Contain    ${ps.stdout}    lmmd

Kill Worker Process
    [Documentation]    Force-kill a worker, no cleanup (SIGKILL semantics;
    ...    `taskkill /F` on Windows). The supervisor must treat any
    ...    unexpected death as fatal regardless of the exit code shape.
    [Arguments]    ${pid}
    ${sys}=    Evaluate    platform.system()    modules=platform
    IF    $sys == 'Windows'
        ${kill}=    Run Process    taskkill    /F    /PID    ${pid}
    ELSE
        ${kill}=    Run Process    kill    -9    ${pid}
    END
    Should Be Equal As Integers    ${kill.rc}    0


*** Test Cases ***
Worker Crash Exits Daemon Non-Zero
    [Documentation]    SIGKILL one worker; the supervisor must exit non-zero
    ...    (fail-fast, no silent masking).
    ${pid}=    Worker Pid    CrashBob
    Worker Process Should Be An Lmmd Worker    ${pid}
    Kill Worker Process    ${pid}
    ${res}=    Wait For Process    lmmd    timeout=30s
    Should Not Be Equal As Integers    ${res.rc}    0
