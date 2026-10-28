find_package(Qt6 6.2 REQUIRED COMPONENTS Test)

# Thin wrapper: only the invariant shared by every unit test lives here
# (link lmc_core + Qt6::Test, tests/ output dir, add_test registration,
# aggregate list for CI). All variability stays at the call site as plain
# CMake: resource.qrc as an extra source in the call, fixture dirs via
# target_compile_definitions, QT_QPA_PLATFORM=offscreen via
# set_tests_properties duplicated per test on purpose (explicit over hidden
# default; singleinstance simply omits it).
# Adding a test = one lmc_add_unit_test(...) call + its variability lines.
# CI builds `lmc_unit_tests` and runs plain `ctest` (no -R), so build.yml
# never changes for new tests.
function(lmc_add_unit_test TEST_NAME)
    add_executable(tst_${TEST_NAME} ${ARGN})
    target_link_libraries(tst_${TEST_NAME} PRIVATE lmc_core Qt6::Test)
    set_target_properties(tst_${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
    )
    add_test(NAME ${TEST_NAME} COMMAND tst_${TEST_NAME})
    set_property(GLOBAL APPEND PROPERTY LMC_UNIT_TEST_TARGETS tst_${TEST_NAME})
endfunction()

# Crypto unit tests: pure lmcCrypto round-trips, no network, no GUI.
# Fixture dir is baked in at configure time (out-of-source safe).
lmc_add_unit_test(crypto unit/tst_crypto.cpp)
target_compile_definitions(tst_crypto PRIVATE
    LMC_CRYPTO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/crypto"
)
# tst links Qt Widgets (via lmc_core) so QTest instantiates QApplication:
# no X server needed for headless crypto checks.
set_tests_properties(crypto PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Single-instance election: QLockFile + QLocalServer, no GUI. The binary
# re-executes itself as a helper child (QProcess), so crash-takeover and
# transient-holder races are covered without POSIX-only primitives.
# QCoreApplication only: no display needed at all.
lmc_add_unit_test(singleinstance unit/tst_singleinstance.cpp)

# Second-instance argv mapping: /term -> "term", everything else -> "raise".
# Links lmc_core for remoteCommandFor (apprunner.cpp), offscreen for QWidget.
lmc_add_unit_test(apprunner unit/tst_apprunner.cpp)
set_tests_properties(apprunner PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# macOS Dock/Finder reopen: in-process ApplicationActivate -> messageReceived.
# Owns an Application (QApplication) instance: offscreen like apprunner.
lmc_add_unit_test(application unit/tst_application.cpp)
set_tests_properties(application PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Theme validator (native chat render): pure JSON validation,
# no network, no GUI. Fixture dir is the source themes dir.
lmc_add_unit_test(theme unit/tst_theme.cpp ../src/resource.qrc)
target_compile_definitions(tst_theme PRIVATE
    LMC_THEME_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src/resources/themes"
)
set_tests_properties(theme PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ChatMessageModel: grouping/plainText/files, no network.
lmc_add_unit_test(chatmodel unit/tst_chatmodel.cpp ../src/resource.qrc)
set_tests_properties(chatmodel PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ChatMessageView keyboard nav (Tab/Return over file anchors), offscreen.
lmc_add_unit_test(chatnav unit/tst_chatnav.cpp ../src/resource.qrc)
set_tests_properties(chatnav PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Pure chat units (RowResolver/BubbleGeometry/RowHtmlBuilder), offscreen.
lmc_add_unit_test(chatpure unit/tst_chatpure.cpp)
set_tests_properties(chatpure PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# BubbleDelegate header typography (theme + local size, no message font leak),
# offscreen.
lmc_add_unit_test(bubbleheader unit/tst_bubbleheader.cpp ../src/resource.qrc)
set_tests_properties(bubbleheader PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Right-click + Select All targets the bubble under the cursor, offscreen.
lmc_add_unit_test(chatselectall unit/tst_chatselectall.cpp ../src/resource.qrc)
set_tests_properties(chatselectall PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# History file round-trip (save/getList/getMessage + legacy view row),
# offscreen.
lmc_add_unit_test(history unit/tst_history.cpp ../src/resource.qrc)
set_tests_properties(history PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Incoming XN_FONT/XN_COLOR guard (size clamp, opaque colors, contrast
# floor), offscreen.
lmc_add_unit_test(chattextguard unit/tst_chattextguard.cpp ../src/resource.qrc)
set_tests_properties(chattextguard PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Aggregate for CI: `cmake --build build --target lmc_unit_tests` builds every
# unit test without listing them in build.yml. This aggregate itself is not
# part of ALL; the tst_* executables stay in ALL as before. `package-local`
# stays small because it does not depend on tests, not because of this target.
get_property(_lmc_unit_test_targets GLOBAL PROPERTY LMC_UNIT_TEST_TARGETS)
add_custom_target(lmc_unit_tests DEPENDS ${_lmc_unit_test_targets})
