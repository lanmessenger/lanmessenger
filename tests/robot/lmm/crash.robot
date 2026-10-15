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

Worker Cmdline
    [Documentation]    Full command line of a pid (to verify the kill target).
    ...    Unix: portable `ps -p PID -o args` (GNU-only `--no-headers`
    ...    breaks macOS; the header is harmless, the caller matches with
    ...    Should Contain). Windows: CIM query by ProcessId (no ps/pgrep
    ...    there; run under `powershell`, not pwsh, which may be absent).
    [Arguments]    ${pid}
    ${sys}=    Evaluate    platform.system()    modules=platform
    IF    '${sys}' == 'Windows'
        ${ps}=    Run Process    powershell    -NoProfile    -c
        ...    (Get-CimInstance Win32_Process -Filter 'ProcessId=${pid}').CommandLine
        ...    stdout=${OUTPUT_DIR}/ps-${pid}.txt
    ELSE
        ${ps}=    Run Process    ps    -p    ${pid}    -o    args
        ...    stdout=${OUTPUT_DIR}/ps-${pid}.txt
    END
    Should Be Equal As Integers    ${ps.rc}    0
    RETURN    ${ps.stdout}

Find Worker Pid
    [Documentation]    PID of the worker process serving a peer handle.
    ...    Unix: `pgrep -f`; Windows: CIM query matching the worker
    ...    command line (the supervisor itself has no --worker flag).
    [Arguments]    ${peer}
    ${sys}=    Evaluate    platform.system()    modules=platform
    IF    '${sys}' == 'Windows'
        ${pg}=    Run Process    powershell    -NoProfile    -c
        ...    (Get-CimInstance Win32_Process -Filter "Name='lmmd.exe'" | Where-Object { $_.CommandLine -like '*--worker --peer ${peer}*' }).ProcessId
        ...    stdout=${OUTPUT_DIR}/pgrep-${peer}.txt
    ELSE
        ${pg}=    Run Process    pgrep    -f    lmmd --worker --peer ${peer}
        ...    stdout=${OUTPUT_DIR}/pgrep-${peer}.txt
    END
    Should Be Equal As Integers    ${pg.rc}    0
    ${words}=    Evaluate    """${pg.stdout}""".split()
    Should Not Be Empty    ${words}
    RETURN    ${words}[0]

Kill Worker Process
    [Documentation]    Force-kill a worker, no cleanup (SIGKILL semantics;
    ...    `taskkill /F` on Windows). The supervisor must treat any
    ...    unexpected death as fatal regardless of the exit code shape.
    [Arguments]    ${pid}
    ${sys}=    Evaluate    platform.system()    modules=platform
    IF    '${sys}' == 'Windows'
        ${kill}=    Run Process    taskkill    /F    /PID    ${pid}
    ELSE
        ${kill}=    Run Process    kill    -9    ${pid}
    END
    Should Be Equal As Integers    ${kill.rc}    0


*** Test Cases ***
Worker Crash Exits Daemon Non-Zero
    [Documentation]    SIGKILL one worker; the supervisor must log the peer
    ...    name and exit non-zero (fail-fast, no silent masking).
    ${pid}=    Find Worker Pid    CrashBob
    ${cmd}=    Worker Cmdline    ${pid}
    Should Contain    ${cmd}    --worker --peer CrashBob
    Kill Worker Process    ${pid}
    ${res}=    Wait For Process    lmmd    timeout=30s
    Should Not Be Equal As Integers    ${res.rc}    0
