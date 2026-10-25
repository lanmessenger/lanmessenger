find_package(Qt6 6.2 REQUIRED COMPONENTS Test)

# Crypto unit tests: pure lmcCrypto round-trips, no network, no GUI.
# Fixture dir is baked in at configure time (out-of-source safe).
add_executable(tst_crypto
    unit/tst_crypto.cpp
)

target_compile_definitions(tst_crypto PRIVATE
    LMC_CRYPTO_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/crypto"
)

target_link_libraries(tst_crypto PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_crypto PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME crypto COMMAND tst_crypto)
# tst links Qt Widgets (via lmc_core) so QTest instantiates QApplication:
# no X server needed for headless crypto checks.
set_tests_properties(crypto PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Single-instance election: QLockFile + QLocalServer, no GUI. The binary
# re-executes itself as a helper child (QProcess), so crash-takeover and
# transient-holder races are covered without POSIX-only primitives.
# QCoreApplication only: no display needed at all.
add_executable(tst_singleinstance
    unit/tst_singleinstance.cpp
)

target_link_libraries(tst_singleinstance PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_singleinstance PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME singleinstance COMMAND tst_singleinstance)

# Second-instance argv mapping: /term -> "term", everything else -> "raise".
# Links lmc_core for remoteCommandFor (apprunner.cpp), offscreen for QWidget.
add_executable(tst_apprunner
    unit/tst_apprunner.cpp
)

target_link_libraries(tst_apprunner PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_apprunner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME apprunner COMMAND tst_apprunner)
set_tests_properties(apprunner PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# macOS Dock/Finder reopen: in-process ApplicationActivate -> messageReceived.
# Owns an Application (QApplication) instance: offscreen like apprunner.
add_executable(tst_application
    unit/tst_application.cpp
)

target_link_libraries(tst_application PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_application PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME application COMMAND tst_application)
set_tests_properties(application PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Theme validator (native chat render): pure JSON validation,
# no network, no GUI. Fixture dir is the source themes dir.
add_executable(tst_theme
    unit/tst_theme.cpp
    ../src/resource.qrc
)

target_compile_definitions(tst_theme PRIVATE
    LMC_THEME_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src/resources/themes"
)

target_link_libraries(tst_theme PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_theme PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME theme COMMAND tst_theme)
set_tests_properties(theme PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ChatMessageModel: grouping/plainText/files, no network.
add_executable(tst_chatmodel
    unit/tst_chatmodel.cpp
    ../src/resource.qrc
)

target_link_libraries(tst_chatmodel PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_chatmodel PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME chatmodel COMMAND tst_chatmodel)
set_tests_properties(chatmodel PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ChatMessageView keyboard nav (Tab/Return over file anchors), offscreen.
add_executable(tst_chatnav
    unit/tst_chatnav.cpp
    ../src/resource.qrc
)

target_link_libraries(tst_chatnav PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_chatnav PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME chatnav COMMAND tst_chatnav)
set_tests_properties(chatnav PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# BubbleDelegate header typography (theme + local size, no message font leak),
# offscreen.
add_executable(tst_bubbleheader
    unit/tst_bubbleheader.cpp
    ../src/resource.qrc
)

target_link_libraries(tst_bubbleheader PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_bubbleheader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME bubbleheader COMMAND tst_bubbleheader)
set_tests_properties(bubbleheader PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Incoming XN_FONT/XN_COLOR guard (size clamp, opaque colors, contrast
# floor), offscreen.
add_executable(tst_chattextguard
    unit/tst_chattextguard.cpp
    ../src/resource.qrc
)

target_link_libraries(tst_chattextguard PRIVATE lmc_core Qt6::Test)

set_target_properties(tst_chattextguard PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME chattextguard COMMAND tst_chattextguard)
set_tests_properties(chattextguard PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
