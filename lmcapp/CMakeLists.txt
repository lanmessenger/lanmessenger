find_package(Qt5 5.15 REQUIRED COMPONENTS Core Network Widgets)

# qtlockedfile*.cpp are not compiled standalone: qmake drops them from SOURCES
# as well, they are textually included into qtlocalpeer.cpp inside the
# QtLP_Private namespace (see qtlocalpeer.cpp:65-71).
add_library(lmcapp SHARED
    src/application.cpp
    src/application.h
    src/qtlocalpeer.cpp
    src/qtlocalpeer.h
    src/qtlockedfile.h
    src/qtsingleapplication.cpp
    src/qtsingleapplication.h
    src/qtsinglecoreapplication.cpp
    src/qtsinglecoreapplication.h
)

# Order matters: when building the library, its own headers (src/) must win
# over the consumer copies (include/) — the src/ variant marks the classes
# __declspec(dllexport) on Windows.
target_include_directories(lmcapp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_definitions(lmcapp PRIVATE
    LMCAPP_LIBRARY
    QT_DISABLE_DEPRECATED_BEFORE=0x050F00
)

target_link_libraries(lmcapp
    PUBLIC
        Qt5::Core
        Qt5::Network
        Qt5::Widgets
)

if(WIN32)
    target_sources(lmcapp PRIVATE src/lmcappwin32.rc)
    # qmake: skip_target_version_ext — no version suffix (lmcapp.dll/lmcapp.lib),
    # the CI hack "lmcapp2.lib -> lmcapp.lib" is no longer needed.
    set_target_properties(lmcapp PROPERTIES PREFIX "" OUTPUT_NAME lmcapp)
else()
    # liblmcapp.so.2.0.0 with SONAME liblmcapp.so.2 (buildx11 copies .so.2),
    # on macOS: liblmcapp.2.0.0.dylib.
    set_target_properties(lmcapp PROPERTIES VERSION 2.0.0 SOVERSION 2)
endif()

if(APPLE)
    # buildmacos ships liblmcapp.2.dylib next to the app binary and rewrites the
    # load path to @executable_path with install_name_tool; bake that install
    # name in at link time instead.
    set_target_properties(lmcapp PROPERTIES
        MACOSX_RPATH OFF
        INSTALL_NAME_DIR "@executable_path"
    )
endif()

set_target_properties(lmcapp PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Runtime install is part of the release layout rules in lmc/CMakeLists.txt
# (qmake's INSTALLS += target -> /usr/local/lib is unused; packages are built
# by lmc/setup/** from the release layout).
