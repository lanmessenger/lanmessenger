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
    # relies on it (qtlockedfile_win.cpp, shared.cpp getLogonName).
    add_compile_definitions(UNICODE _UNICODE)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_subdirectory(lmcapp)
add_subdirectory(lmc)

enable_testing()
add_subdirectory(tests)
