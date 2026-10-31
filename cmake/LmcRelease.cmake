# LmcRelease.cmake — install() rules + package-local target.
#
# Included from src/CMakeLists.txt (NOT from the top level): install(TARGETS
# lmc) belongs to the directory that defines the lmc target, and include()
# preserves that directory scope. Uses LMC_RUNTIME_DIR / LMC_RES_DIR set by
# src/ and LmcQtPlugins for QT_PLUGINS_DIR discovery.
#
# Result of `cmake --build <build-dir> --target package-local` is the CLEAN
# system payload in LMC_RELEASE_DIR (defaults to <build>/release): binary,
# lmc.rcc, lang, sounds — no Qt plugin copies on Linux (system packages use
# the distro Qt via setup/linux/nfpm.yaml dependencies). On macOS/Windows
# this is the full bundle. Hermetic Qt plugin copies for local runs/e2e live
# only in build/teststage (see cmake/LmcTeststage.cmake), never in release/.

if(APPLE)
    # macOS: everything goes into LAN-Messenger.app/Contents/MacOS and Qt
    # imageformat plugins into Contents/Plugins (see main.cpp setLibraryPaths).
    set(LMC_LAYOUT_BIN_DIR LAN-Messenger.app/Contents/MacOS)
    set(LMC_LAYOUT_LICENSE COPYING)
elseif(WIN32)
    # Windows: the license file is named COPYING.txt.
    set(LMC_LAYOUT_BIN_DIR .)
    set(LMC_LAYOUT_LICENSE COPYING.txt)
else()
    set(LMC_LAYOUT_BIN_DIR .)
    set(LMC_LAYOUT_LICENSE COPYING)
endif()

if(APPLE)
    install(TARGETS lmc BUNDLE DESTINATION .)
else()
    install(TARGETS lmc RUNTIME DESTINATION .)
endif()

install(FILES ${LMC_RUNTIME_DIR}/lmc.rcc DESTINATION ${LMC_LAYOUT_BIN_DIR})
# The app's own lang/*.qm (lrelease) plus Qt's lang/system/*.qm (copied at
# configure time from the Qt used for the build).
install(DIRECTORY ${LMC_RUNTIME_DIR}/lang/ DESTINATION ${LMC_LAYOUT_BIN_DIR}/lang
        FILES_MATCHING PATTERN "*.qm")
# Notification sounds. Chat themes ship embedded (:/themes) + user dir,
# not staged here.
install(DIRECTORY ${LMC_RES_DIR}/sounds DESTINATION ${LMC_LAYOUT_BIN_DIR})
install(FILES ${LMC_RES_DIR}/text/license.txt DESTINATION ${LMC_LAYOUT_BIN_DIR}
        RENAME ${LMC_LAYOUT_LICENSE})

# Qt plugins live in the Qt installation's plugins/ directory. Queried with
# the Qt tools instead of guessing relative paths (see LmcQtPlugins.cmake).
# Only needed for bundles: macOS app, Windows deploy, and the Linux teststage;
# the clean Linux release tree carries no plugin copies.
include(LmcQtPlugins)
lmc_resolve_qt_plugins()
if(NOT QT_PLUGINS_DIR)
    if(UNIX AND NOT APPLE)
        message(WARNING
            "Could not locate the Qt plugin directory (qtpaths/qmake query failed); "
            "clean release still stages, but the teststage cannot bundle plugins. "
            "Pass -DQT_PLUGINS_DIR=<path>.")
    else()
        message(FATAL_ERROR
            "Could not locate the Qt plugin directory (qtpaths/qmake query failed); "
            "pass -DQT_PLUGINS_DIR=<path>")
    endif()
endif()
# Expected QSQLITE driver for this platform. A missing driver only warns at
# configure time so build/ctest keep working on minimal Qt installs.
# The hermetic teststage (Linux) bundles it from QT_PLUGINS_DIR and fails
# fast there when absent (see cmake/LmcTeststage.cmake); system deb/rpm packages
# use the distro driver via package dependencies instead.
if(NOT EXISTS "${LMC_QT_SQLITE_DRIVER}")
    message(WARNING
        "QSQLite driver not found at ${LMC_QT_SQLITE_DRIVER} "
        "(HistoryStore needs Qt6 Sql with the SQLite plugin; Debian/Ubuntu: "
        "libqt6sql6-sqlite). The Linux teststage requires it; install "
        "the plugin or reconfigure with -DQT_PLUGINS_DIR=<path> pointing at "
        "the directory containing sqldrivers/.")
endif()
if(APPLE)
    foreach(_lmc_plugin ${LMC_QT_IMAGEFORMAT_PLUGINS})
        install(FILES ${_lmc_plugin}
                DESTINATION LAN-Messenger.app/Contents/Plugins/imageformats)
    endforeach()
    # TIFF moved out of qtbase in Qt 6 (qtimageformats module); macOS shipped
    # it historically, so take it when present and skip silently otherwise.
    install(FILES ${QT_PLUGINS_DIR}/imageformats/libqtiff.dylib
            DESTINATION LAN-Messenger.app/Contents/Plugins/imageformats
            OPTIONAL)
    # NOTE: no sqldrivers here — stage-mac-app wipes Contents/Plugins and
    # macdeployqt stages the SQLite driver itself (gated by verify-dmg.yml).
endif()
# NOTE (Linux): the clean release tree carries NO Qt plugin copies
# (imageformats/platforms/tls/sqldrivers). System deb/rpm packages depend
# on the distro Qt instead (see setup/linux/nfpm.yaml). The hermetic
# copies for local runs/e2e live only in build/teststage (see
# cmake/LmcTeststage.cmake), bundled there from QT_PLUGINS_DIR.

if(WIN32)
    # OpenSSL DLLs from the openssl/bin redistributable.
    file(GLOB _lmc_openssl_dlls
        ${CMAKE_SOURCE_DIR}/openssl/bin/libcrypto*.dll
        ${CMAKE_SOURCE_DIR}/openssl/bin/libssl*.dll)
    if(_lmc_openssl_dlls)
        install(FILES ${_lmc_openssl_dlls} DESTINATION .)
    endif()
    # NOTE: no sqldrivers here — windeployqt stages qsqlite.dll (lmc links
    # Qt6::Sql). CI asserts it (build.yml) and the installer gate requires
    # it (verify-nsis.yml), so cmake --install stays usable without a
    # SQLite-capable Qt for build/ctest.
endif()

if(UNIX AND NOT APPLE)
    # Launcher script (sets LD_LIBRARY_PATH) and the Unity tray whitelist helper.
    install(PROGRAMS
        scripts/lan-messenger.sh
        scripts/whitelist
        DESTINATION .
    )
endif()

# package-local: stage the layout in one command. LMC_RELEASE_DIR
# defaults to <build>/release; CI points it at release where setup/**
# expects the layout. On Linux this is the CLEAN system payload (no Qt
# plugin copies); the hermetic copies for dev/e2e are added only in
# build/teststage. On macOS/Windows this is the full bundle.
set(LMC_RELEASE_DIR "${CMAKE_BINARY_DIR}/release" CACHE PATH
    "Release layout staging directory for the package-local target")
add_custom_target(package-local
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}
            --config $<CONFIG> --prefix ${LMC_RELEASE_DIR}
    COMMENT "Staging release layout into ${LMC_RELEASE_DIR}"
    VERBATIM
)
add_dependencies(package-local lmc lmc_runtime_files)
