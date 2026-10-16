add_executable(lmmd
    daemon/main.cpp
    daemon/lmmd.cpp
    daemon/lmmd.h
    daemon/supervisor.cpp
    daemon/supervisor.h
    daemon/rpc.h
)

target_compile_definitions(lmmd PRIVATE
    LMM_VERSION="0.1.0"
)

# lmc_core propagates Qt (Core/Gui/Widgets/Network/Xml) + OpenSSL publicly.
target_link_libraries(lmmd PRIVATE lmc_core)

set_target_properties(lmmd PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools/lmm
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tools/lmm
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tools/lmm
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tools/lmm
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tools/lmm
)
