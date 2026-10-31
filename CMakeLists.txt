cmake_minimum_required(VERSION 3.16)

project(lan-messenger VERSION 2.0.0 LANGUAGES CXX)

# Display version: git tag (vX.Y.Z) via -DLMC_VERSION, else 0.0.0.
set(LMC_VERSION "0.0.0" CACHE STRING "Application display version (defaults to git tag)")
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

# Shared staging modules (cmake/LmcQtPlugins.cmake, LmcRelease.cmake,
# LmcTeststage.cmake) are included by src/ and below.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

# ---------------------------------------------------------------------------
# teststage (A1): see cmake/LmcTeststage.cmake. Local mirror of the CI test
# layout; release/ stays immutable, the e2e/robot stand is assembled as a
# copy in build/teststage/. Included here (after src/tests/tools) so the
# package-local, lan-messenger-test and lmmd targets already exist.
# ---------------------------------------------------------------------------
include(LmcTeststage)
