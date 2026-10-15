*** Settings ***
Documentation     Per-peer state isolation of the supervisor+workers daemon:
...               each peer owns its HOME (lmc.ini with its own Name/Status),
...               display names stay per-peer on the wire, and peer.remove
...               cleans only its own dir. Mock-only, no display needed.
Library           OperatingSystem
Library           Collections
Resource          ../mockstand.resource
Suite Setup       Start Isolation Stand
Suite Teardown    Stop Mock Stand


*** Variables ***
${SOCKET}         lmm_iso
${PEER_HOME}      ${OUTPUT_DIR}/lmmd-home-isolation


*** Keywords ***
Start Isolation Stand
    [Documentation]    Daemon + two peers with distinct statuses, cross-visible.
    Start Mock Stand    ${SOCKET}    ${PEER_HOME}
    Add Peer    IsoAlice    chat
    Add Peer    IsoBob    busy
    Wait For Contact    IsoAlice    IsoBob    timeout=30s
    Wait For Contact    IsoBob    IsoAlice    timeout=30s
    Reset Event Cursor


*** Test Cases ***
Peer Homes Are Separate
    [Documentation]    Each peer has its own lmc.ini with its own Name/Status
    ...    (no shared-HOME cross-pollution).
    ${a}=    Get File    ${PEER_HOME}/peers/IsoAlice/.config/lmc/lmc.ini
    ${b}=    Get File    ${PEER_HOME}/peers/IsoBob/.config/lmc/lmc.ini
    Should Contain    ${a}    Name=IsoAlice
    Should Contain    ${a}    Status=chat
    Should Contain    ${b}    Name=IsoBob
    Should Contain    ${b}    Status=busy

Peer List Carries Per-Peer Identity
    [Documentation]    Handles, display names and userIds are distinct per peer.
    ${peers}=    List Peers
    Length Should Be    ${peers}    2
    ${names}=    Evaluate    sorted(p["peer"] for p in $peers)
    ${displays}=    Evaluate    sorted(p["displayName"] for p in $peers)
    ${ids}=    Evaluate    set(p["userId"] for p in $peers)
    Should Be Equal    ${names}[0]    IsoAlice
    Should Be Equal    ${names}[1]    IsoBob
    Should Be Equal    ${displays}[0]    IsoAlice
    Should Be Equal    ${displays}[1]    IsoBob
    Length Should Be    ${ids}    2

Mixed Statuses Still Deliver
    [Documentation]    Distinct statuses do not break discovery or delivery.
    Send Message    IsoAlice    IsoBob    e2e-iso-001
    ${ev}=    Wait For Event    message.received    peer=IsoBob
    ...    timeout=15s    text=e2e-iso-001
    Should Be Equal    ${ev}[fromName]    IsoAlice

Peer Remove Cleans Only Its Dir
    [Documentation]    peer.remove reaps the worker and deletes only its
    ...    home; the surviving peer keeps working.
    Remove Peer    IsoBob
    Directory Should Not Exist    ${PEER_HOME}/peers/IsoBob
    Directory Should Exist    ${PEER_HOME}/peers/IsoAlice
    ${peers}=    List Peers
    Length Should Be    ${peers}    1
    Should Be Equal    ${peers}[0][peer]    IsoAlice
    Announce Peer    IsoAlice
