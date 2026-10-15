# In-process GUI test agent (see PROTOCOL.md and todo/lmc-test-adapter.md).
#
# One shared core, two embodiments (same code, no duplication):
# - lmc_test_preload (.so, phase A): LD_PRELOAD into the unmodified
#   production binary; the server starts in the GUI thread via the
#   constructor in preload.cpp.
# - lan-messenger-test (phase B): test binary with the agent linked in
#   (test-main.cpp, same lmc_core as production), no preload needed.
#   Behind LMC_WITH_TEST_AGENT so weak VMs can skip the extra link.

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Network)

add_library(lmc_agent_core STATIC
    agent_core.cpp
    agent_server.cpp
    agent_core.h
    agent_server.h
)

target_link_libraries(lmc_agent_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
)

target_include_directories(lmc_agent_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(lmc_agent_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(lmc_test_preload SHARED
    preload.cpp
)

target_link_libraries(lmc_test_preload PRIVATE lmc_agent_core)

set_target_properties(lmc_test_preload PROPERTIES
    OUTPUT_NAME lmc_test_preload
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/lmc-agent
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests/lmc-agent
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests/lmc-agent
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests/lmc-agent
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests/lmc-agent
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/lmc-agent
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests/lmc-agent
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests/lmc-agent
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests/lmc-agent
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests/lmc-agent
)

option(LMC_WITH_TEST_AGENT "Build the lan-messenger-test binary (phase B)" ON)
if(LMC_WITH_TEST_AGENT)
    # resource.qrc repeats here (same as the lmc executable and tst_lmc_gui):
    # the Qt resource initializers must live in an object that is always
    # linked, and a static lib would drop an unreferenced qrc object.
    add_executable(lan-messenger-test
        test-main.cpp
        ${CMAKE_SOURCE_DIR}/lmc/src/resource.qrc
    )

    target_link_libraries(lan-messenger-test PRIVATE lmc_core lmc_agent_core)

    set_target_properties(lan-messenger-test PROPERTIES
        OUTPUT_NAME lan-messenger-test
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/lmc-agent
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests/lmc-agent
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests/lmc-agent
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests/lmc-agent
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests/lmc-agent
    )
endif()
