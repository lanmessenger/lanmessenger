find_package(Qt6 6.4 REQUIRED COMPONENTS Test)

# resource.qrc repeats here (same as the lmc executable): the Qt resource
# initializers must live in an object that is always linked, and a static lib
# would drop an unreferenced qrc object.
add_executable(tst_lmc_gui
    e2e/tst_lmc_gui.cpp
    ${CMAKE_SOURCE_DIR}/lmc/src/resource.qrc
)

target_link_libraries(tst_lmc_gui PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_lmc_gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)
