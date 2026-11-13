cmake_minimum_required(VERSION 3.16)

project(lmc VERSION 2.0.0 LANGUAGES CXX)

# Display version: git tag (vX.Y.Z) via -DLMC_VERSION, else 0.0.0.
# Windows numeric stamps derive from this in src/CMakeLists.txt.
set(LMC_VERSION "0.0.0" CACHE STRING "Application display version (defaults to git tag)")
# Single accepted format: no leading v (tags are vX.Y.Z, pass X.Y.Z).
if(LMC_VERSION MATCHES "^[vV]")
  message(FATAL_ERROR "LMC_VERSION '${LMC_VERSION}' must not have a leading v (tags are vX.Y.Z, pass X.Y.Z)")
endif()
message(STATUS "LMC version: ${LMC_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile_commands.json for clangd / IDEs.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(WIN32)
    enable_language(RC)
    # win32-msvc mkspec defines UNICODE/_UNICODE (TCHAR is wchar_t); the code
    # relies on it (shared.cpp getLogonName).
    add_compile_definitions(UNICODE _UNICODE)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)

# lmm test daemon. OFF on weak VMs to keep the default build small
# (cmake --build build --target lmmd still works when OFF).
option(LMC_BUILD_LMM "Build the lmm test daemon (tools/lmm)" ON)
if(LMC_BUILD_LMM)
    add_subdirectory(tools/lmm)
endif()

# In-process GUI test agent (the lan-messenger-test binary).
# Test tooling lives in tools/ next to the lmm mock daemon; tests/ holds
# only the suites themselves. See tools/lmc-agent/PROTOCOL.md.
add_subdirectory(tools/lmc-agent)
