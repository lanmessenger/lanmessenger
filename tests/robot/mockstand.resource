*** Settings ***
Documentation     Shared keywords for suites that need the lmmd mock-peer
...               daemon (tools/lmm). Split from common.resource on purpose:
...               common.resource owns the application side (Launch/Close
...               LMC through the in-process agent), this file owns the mock
...               side (daemon lifecycle through LmmLibrary). Suites that
...               drive only the app never import this file.
Library           Process
Library           OperatingSystem
# Path from this file's own directory: the mock driver lives next to
# the lmm/ self-check suites that were its first consumers.
Library           lmm/LmmLibrary.py


*** Variables ***
# Overridden from CI via `-v LMMD_BIN:...`. Default points at the local
# build tree (cmake -S . -B build, target lmmd), anchored at this file's
# own ${CURDIR} so suite moves never break the depth.
${LMMD_BIN}       ${CURDIR}/../build/tools/lmm/lmmd


*** Keywords ***
Start Mock Stand
    [Documentation]    Launch the daemon with an isolated home dir and
    ...    connect to it. Peers, cross-visibility and the event cursor are
    ...    the caller's job: each suite needs its own peer set, and Reset
    ...    Event Cursor only makes sense after the setup chatter.
    ...    ${home} must live under ${TEMPDIR}, never under ${OUTPUT_DIR}:
    ...    the workers store received files below their HOME (Documents /
    ...    Received Files), and everything under OUTPUT_DIR is uploaded as
    ...    CI artifacts — a home under OUTPUT_DIR leaks test payloads
    ...    (e.g. the files.robot bins, up to 32MB) into robot-results-*.
    ...    The dir is wiped on start: full and --rerunfailed runs share
    ...    TEMPDIR, so a stale peer dir must not survive into the retry.
    [Arguments]    ${port}    ${home}
    Run Keyword And Ignore Error    Remove Directory    ${home}    recursive=True
    Create Directory    ${home}
    # TCP loopback, one address namespace on every OS: no socket files,
    # no per-OS path resolution, nothing to clean up after a crash.
    Start Process    ${LMMD_BIN}    --port    ${port}
    ...    --home    ${home}    alias=lmmd
    ...    stdout=${OUTPUT_DIR}/lmmd-${port}-stdout.txt    stderr=STDOUT
    Connect To Daemon    ${port}    timeout=10s

Stop Mock Stand
    [Documentation]    Clean shutdown: peers send Depart.
    ...    Everything guarded: teardown must not mask test results.
    Run Keyword And Ignore Error    Shutdown Daemon
    Run Keyword And Ignore Error    Disconnect From Daemon
    Run Keyword And Ignore Error    Terminate Process    lmmd
