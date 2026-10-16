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
