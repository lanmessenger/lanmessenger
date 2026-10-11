find_package(Qt6 6.2 REQUIRED COMPONENTS Core Network Widgets)

# qtlockedfile*.cpp are not compiled standalone: they are textually included
# into qtlocalpeer.cpp inside the QtLP_Private namespace
# (see qtlocalpeer.cpp:65-71).
#
# OBJECT, not SHARED: lmcapp has a single consumer (lmc_core)
add_library(lmcapp OBJECT
    src/application.cpp
    src/application.h
    src/qtlocalpeer.cpp
    src/qtlocalpeer.h
    src/qtlockedfile.h
    src/qtsingleapplication.cpp
    src/qtsingleapplication.h
)

# Single header set in src/, exposed to the consumer build.
target_include_directories(lmcapp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_compile_definitions(lmcapp PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

target_link_libraries(lmcapp
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Widgets
)

# No install() rules: an OBJECT library produces no runtime artifact of its
# own. The release layout is staged by lmc/CMakeLists.txt; packages are built
# by lmc/setup/** from that layout (no system-wide install).
