# LmcTeststage.cmake — teststage target (A1).
#
# Included from the top-level CMakeLists.txt, AFTER add_subdirectory(src /
# tests / tools) so that the package-local, lan-messenger-test and lmmd
# targets already exist. Local mirror of the CI test layout ("Pack test
# layout" in build.yml: $STAGE/release + $STAGE/build/tools/lmm).
#
# release/ stays immutable (read-only source); the e2e/robot stand is
# assembled by copying release/ into build/teststage/release/ and adding to
# the copy: test binaries always, Qt plugin copies on Linux
# (imageformats/platforms/tls/sqldrivers from QT_PLUGINS_DIR) — never into
# release/ itself. A future tar is a plain `tar -C teststage release build`.

set(LMC_TESTSTAGE_DIR "${CMAKE_BINARY_DIR}/teststage" CACHE PATH
    "Test-stage directory for the e2e/robot stand (copy of release + test binaries)")
if(LMC_TESTSTAGE_DIR STREQUAL LMC_RELEASE_DIR)
  message(FATAL_ERROR
      "LMC_TESTSTAGE_DIR (${LMC_TESTSTAGE_DIR}) must differ from "
      "LMC_RELEASE_DIR (${LMC_RELEASE_DIR}): the stage is a copy, not the source.")
endif()

set(_lmc_teststage_depends package-local)
if(TARGET lan-messenger-test)
  list(APPEND _lmc_teststage_depends lan-messenger-test)
endif()
if(TARGET lmmd)
  list(APPEND _lmc_teststage_depends lmmd)
endif()

add_custom_target(teststage
    DEPENDS ${_lmc_teststage_depends}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${LMC_TESTSTAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_TESTSTAGE_DIR}/release
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_TESTSTAGE_DIR}/build/tools/lmm
    # Trailing /. copies CONTENTS (dest exists); without it cp would nest
    # release inside release and the layout would silently break (same as CI).
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${LMC_RELEASE_DIR}/ ${LMC_TESTSTAGE_DIR}/release
    COMMENT "Staging test layout into ${LMC_TESTSTAGE_DIR} (copy of release + test binaries)"
    VERBATIM
)

if(TARGET lan-messenger-test)
  add_custom_command(TARGET teststage POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:lan-messenger-test> ${LMC_TESTSTAGE_DIR}/release/
      COMMENT "Adding lan-messenger-test to the test stage"
      VERBATIM
  )
endif()
if(TARGET lmmd)
  add_custom_command(TARGET teststage POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:lmmd> ${LMC_TESTSTAGE_DIR}/build/tools/lmm/
      COMMENT "Adding lmmd to the test stage"
      VERBATIM
  )
endif()

if(UNIX AND NOT APPLE)
  # Shared Qt plugin discovery (single source of truth with the release
  # layout); the hermetic copies below come from these variables.
  include(LmcQtPlugins)
  lmc_resolve_qt_plugins()
  if(NOT QT_PLUGINS_DIR)
    message(WARNING
        "teststage: QT_PLUGINS_DIR unknown, Qt plugins will not be bundled "
        "(install qt6-base-dev/qtpaths and reconfigure for a hermetic stage)")
  endif()
  # Hermetic teststage on Linux: bundle the Qt plugins next to the binary
  # (QCoreApplication::libraryPaths() includes the application directory).
  # The clean release tree intentionally carries none of these; system
  # packages use the distro Qt via nfpm.yaml dependencies instead.
  add_custom_command(TARGET teststage POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${LMC_TESTSTAGE_DIR}/release/imageformats
              ${LMC_TESTSTAGE_DIR}/release/platforms
              ${LMC_TESTSTAGE_DIR}/release/tls
              ${LMC_TESTSTAGE_DIR}/release/sqldrivers
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${LMC_QT_IMAGEFORMAT_PLUGINS}
              ${LMC_TESTSTAGE_DIR}/release/imageformats/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${LMC_QT_PLATFORM_PLUGIN}
              ${LMC_TESTSTAGE_DIR}/release/platforms/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${LMC_QT_SQLITE_DRIVER}
              ${LMC_TESTSTAGE_DIR}/release/sqldrivers/
      COMMENT "Bundling Qt plugins into the test stage"
      VERBATIM
  )
  if(LMC_QT_TLS_PLUGINS)
    add_custom_command(TARGET teststage POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${LMC_QT_TLS_PLUGINS}
                ${LMC_TESTSTAGE_DIR}/release/tls/
        COMMENT "Bundling Qt TLS backends into the test stage"
        VERBATIM
    )
  endif()
  # Fail fast like CI ("test -e .../sqldrivers/libqsqlite.so"): a driver-less
  # stage boots with an empty history view instead of erroring.
  add_custom_command(TARGET teststage POST_BUILD
      COMMAND sh -c "test ! -e '${LMC_TESTSTAGE_DIR}/release/release' || (echo 'teststage: nested release/release, refusing' >&2; exit 1)"
      COMMAND sh -c "test -e '${LMC_TESTSTAGE_DIR}/release/sqldrivers/libqsqlite.so' || (echo 'teststage: missing sqldrivers/libqsqlite.so in ${LMC_TESTSTAGE_DIR}/release (install libqt6sql6-sqlite and rebuild package-local)' >&2; exit 1)"
      COMMAND sh -c "test -x '${LMC_TESTSTAGE_DIR}/release/lan-messenger-test' || (echo 'teststage: lan-messenger-test missing (LMC_WITH_TEST_AGENT=OFF?)' >&2; exit 1)"
      COMMAND sh -c "test -x '${LMC_TESTSTAGE_DIR}/build/tools/lmm/lmmd' || (echo 'teststage: lmmd missing (LMC_BUILD_LMM=OFF?)' >&2; exit 1)"
      COMMENT "Verifying test stage layout"
      VERBATIM
  )
endif()
