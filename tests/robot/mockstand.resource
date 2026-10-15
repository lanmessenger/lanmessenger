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
    [Arguments]    ${socket}    ${home}
    Create Directory    ${home}
    # Full socket path under the suite output dir: QLocalServer resolves
    # a bare name via QDir::tempPath (($TMPDIR, /var/folders/... on macOS;
    # /tmp on Linux where TMPDIR is unset), so with a bare name daemon and
    # client would disagree across OSes. A name containing '/' is used
    # verbatim as the socket file on all platforms.
    ${sockpath}=    Join Path    ${OUTPUT_DIR}    lmm-${socket}.sock
    Start Process    ${LMMD_BIN}    --socket-name    ${sockpath}
    ...    --home    ${home}    alias=lmmd
    ...    stdout=${OUTPUT_DIR}/lmmd-${socket}-stdout.txt    stderr=STDOUT
    Connect To Daemon    ${sockpath}    timeout=10s

Stop Mock Stand
    [Documentation]    Clean shutdown: peers send Depart, no stale socket.
    ...    Everything guarded: teardown must not mask test results.
    Run Keyword And Ignore Error    Shutdown Daemon
    Run Keyword And Ignore Error    Disconnect From Daemon
    Run Keyword And Ignore Error    Terminate Process    lmmd
