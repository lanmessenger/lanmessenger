find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Network Xml LinguistTools)
if(UNIX)
    find_package(Qt6 6.2 REQUIRED COMPONENTS Multimedia)
endif()
find_package(OpenSSL REQUIRED)

# Generated runtime files (lmc.rcc, *.qm) land here; the flat release layout
# itself is staged from the install() rules at the bottom.
set(LMC_RUNTIME_DIR ${CMAKE_CURRENT_BINARY_DIR}/runtime)
set(LMC_RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

add_library(lmc_core STATIC
    gui/main/aboutdialog.cpp
    gui/chat/broadcastwindow.cpp
    gui/chat/bubbledelegate.cpp
    gui/chat/chathelper.cpp
    gui/chat/chatmessagemodel.cpp
    gui/chat/chatmessageview.cpp
    gui/chat/chatroomwindow.cpp
    gui/chat/chatwindow.cpp
    core/crypto.cpp
    network/datagram.cpp
    messaging/filemessagingproc.cpp
    gui/files/filemodelview.cpp
    gui/main/helpwindow.cpp
    messaging/history.cpp
    gui/history/historywindow.cpp
    gui/widgets/imagepicker.cpp
    gui/widgets/imagepickeraction.cpp
    app/apprunner.cpp
    app/application.cpp
    app/lmc.cpp
    app/singleinstance.cpp
    core/lmcstrings.cpp
    gui/main/mainwindow.cpp
    messaging/message.cpp
    messaging/messaging.cpp
    messaging/messagingproc.cpp
    network/netstreamer.cpp
    network/network.cpp
    core/settings.cpp
    gui/main/settingsdialog.cpp
    core/shared.cpp
    core/soundplayer.cpp
    gui/widgets/subcontrols.cpp
    network/tcpnetwork.cpp
    core/theme.cpp
    core/lmclogging.cpp
    gui/files/transferlistview.cpp
    gui/files/transferwindow.cpp
    network/udpnetwork.cpp
    gui/main/updatewindow.cpp
    gui/contacts/userinfowindow.cpp
    gui/contacts/userselectdialog.cpp
    gui/contacts/usertreewidget.cpp
    network/webnetwork.cpp
    messaging/xmlmessage.cpp
    gui/widgets/actionhelper.h
    gui/main/aboutdialog.h
    gui/chat/broadcastwindow.h
    gui/chat/bubbledelegate.h
    gui/chat/chatdefinitions.h
    gui/chat/chatwindow.h
    gui/chat/chatmessagemodel.h
    gui/chat/chatmessageview.h
    gui/chat/chatroomwindow.h
    core/crypto.h
    network/datagram.h
    core/definitions.h
    gui/files/filemodelview.h
    gui/main/helpwindow.h
    messaging/history.h
    gui/history/historytreewidget.h
    gui/history/historywindow.h
    gui/widgets/imagepicker.h
    gui/widgets/imagepickeraction.h
    app/apprunner.h
    app/application.h
    app/lmc.h
    app/singleinstance.h
    core/lmcstrings.h
    gui/main/mainwindow.h
    messaging/message.h
    messaging/messaging.h
    network/netstreamer.h
    network/network.h
    app/resource.h
    core/settings.h
    gui/main/settingsdialog.h
    core/shared.h
    core/soundplayer.h
    core/stdlocation.h
    gui/widgets/subcontrols.h
    network/tcpnetwork.h
    core/theme.h
    core/lmccategories.h
    core/lmclogging.h
    gui/files/transferlistview.h
    gui/files/transferwindow.h
    core/uidefinitions.h
    network/udpnetwork.h
    gui/main/updatewindow.h
    gui/contacts/userinfowindow.h
    gui/contacts/userselectdialog.h
    gui/contacts/usertreewidget.h
    network/webnetwork.h
    messaging/xmlmessage.h
    gui/main/aboutdialog.ui
    gui/chat/broadcastwindow.ui
    gui/chat/chatroomwindow.ui
    gui/chat/chatwindow.ui
    gui/main/helpwindow.ui
    gui/history/historywindow.ui
    gui/main/mainwindow.ui
    gui/main/settingsdialog.ui
    gui/files/transferwindow.ui
    gui/main/updatewindow.ui
    gui/contacts/userinfowindow.ui
    gui/contacts/userselectdialog.ui
)

target_compile_definitions(lmc_core PUBLIC
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
    "IDA_VERSION=\"${LMC_VERSION}\""
)

# AUTOUIC puts ui_*.h in <AUTOGEN_BUILD_DIR>/include with single-config
# generators and in <AUTOGEN_BUILD_DIR>/include_<CONFIG> with multi-config
# generators (e.g. Visual Studio); expose both so consumers of lmc_core
# (lmc) find them regardless of the generator.
target_include_directories(lmc_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/lmc_core_autogen/include
    ${CMAKE_CURRENT_BINARY_DIR}/lmc_core_autogen/include_$<CONFIG>
)

# Qt6::Sql is not linked (unused; see qt6-migration-plan.md).
target_link_libraries(lmc_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Xml
        OpenSSL::Crypto
)

if(UNIX)
    target_link_libraries(lmc_core PUBLIC Qt6::Multimedia)
endif()

if(WIN32)
    # GetUserNameW(...) in Helper::getLogonName (shared.cpp)
    target_link_libraries(lmc_core PUBLIC advapi32)
endif()

# The executable contributes the entry point and the embedded resource bundle
# (AUTORCC over resource.qrc); test binaries link lmc_core and repeat the
# same .qrc so the Qt resource initializers live in an object that is always
# linked (a static lib would drop an unreferenced qrc object).
add_executable(lmc WIN32
    app/main.cpp
    resource.qrc
)
target_link_libraries(lmc PRIVATE lmc_core)

if(WIN32)
    target_sources(lmc PRIVATE app/lmcwin32.rc)
    set_target_properties(lmc PROPERTIES OUTPUT_NAME lmc)
elseif(APPLE)
    target_sources(lmc PRIVATE app/lmc.icns)
    set_source_files_properties(app/lmc.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    set(LMC_MACOS_SHORT_VERSION "${LMC_VERSION}")
    string(REGEX REPLACE "[~+-].*$" "" LMC_MACOS_SHORT_VERSION
        "${LMC_MACOS_SHORT_VERSION}")
    string(REPLACE "." ";" LMC_MACOS_VERSION_PARTS "${LMC_MACOS_SHORT_VERSION}")
    list(LENGTH LMC_MACOS_VERSION_PARTS LMC_MACOS_VERSION_COUNT)
    if(LMC_MACOS_VERSION_COUNT GREATER 3)
        list(SUBLIST LMC_MACOS_VERSION_PARTS 0 3 LMC_MACOS_VERSION_PARTS)
        list(JOIN LMC_MACOS_VERSION_PARTS "." LMC_MACOS_SHORT_VERSION)
    elseif(LMC_MACOS_VERSION_COUNT LESS 3)
        # Manual versions like 1.2 or 1 must still yield X.Y.Z for Apple.
        while(LMC_MACOS_VERSION_COUNT LESS 3)
            list(APPEND LMC_MACOS_VERSION_PARTS "0")
            list(LENGTH LMC_MACOS_VERSION_PARTS LMC_MACOS_VERSION_COUNT)
        endwhile()
        list(JOIN LMC_MACOS_VERSION_PARTS "." LMC_MACOS_SHORT_VERSION)
    endif()
    # NOTE: CMake MATCHES does not support the {2} repetition quantifier,
    # so spell the three numeric components out explicitly (grep -E in
    # build.yml / verify-dmg.yml keeps the compact {2} form).
    if(NOT LMC_MACOS_SHORT_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
        message(FATAL_ERROR
            "LMC_VERSION '${LMC_VERSION}' does not map to X.Y.Z "
            "(got '${LMC_MACOS_SHORT_VERSION}')")
    endif()
    set(LMC_MACOS_BUNDLE_VERSION "${LMC_VERSION}")
    string(REPLACE "~nightly." "." LMC_MACOS_BUNDLE_VERSION
        "${LMC_MACOS_BUNDLE_VERSION}")
    if(NOT LMC_MACOS_BUNDLE_VERSION MATCHES "^[0-9]+(\\.[0-9]+)*$")
        set(LMC_MACOS_BUNDLE_VERSION "${LMC_MACOS_SHORT_VERSION}")
    endif()
    # File names stay LAN-Messenger (bundle dir, executable); CFBundleName
    # is the short name. What the user sees in Finder/Launchpad/Spotlight
    # is CFBundleDisplayName (see setup/mac/Info.plist.in).
    set(LMC_MACOS_DISPLAY_NAME "LAN Messenger")
    set_target_properties(lmc PROPERTIES
        OUTPUT_NAME "LAN-Messenger"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE lmc.icns
        MACOSX_BUNDLE_BUNDLE_NAME "LAN-Messenger"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../setup/mac/Info.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.lanmessenger.lmc"
        MACOSX_BUNDLE_BUNDLE_VERSION "${LMC_MACOS_BUNDLE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${LMC_MACOS_SHORT_VERSION}"
    )
else()
    set_target_properties(lmc PROPERTIES OUTPUT_NAME lan-messenger)
endif()

# External resource bundle (2nd role of resource.qrc): looked up at runtime as
# <appdir>/lmc.rcc (main sets cwd to applicationDirPath; StdLocation::resourceFile())
# and registered via QResource::registerResource (main.cpp). The 1st role —
# embedding into the binary — is done by AUTORCC (resource.qrc in the sources).
add_custom_command(
    OUTPUT ${LMC_RUNTIME_DIR}/lmc.rcc
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_RUNTIME_DIR}
    COMMAND Qt6::rcc -binary ${CMAKE_CURRENT_SOURCE_DIR}/resource.qrc
            -o ${LMC_RUNTIME_DIR}/lmc.rcc
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resource.qrc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building external resource bundle lmc.rcc"
    VERBATIM
)

# Translations: sources live in <repo>/translations/lmc_<locale>.ts (Qt Linguist
# format, target-prefixed names; adding a file there is enough, this glob picks
# it up (on the Visual Studio generator, re-run configure after add/remove).
# The lrelease output keeps the runtime layout: lang/*.qm
# (sysLangDir = <appdir>/lang, stdlocation.h).
file(GLOB LMC_TS_FILES CONFIGURE_DEPENDS
     ${CMAKE_SOURCE_DIR}/translations/lmc_*.ts)
list(SORT LMC_TS_FILES)

set(LMC_QM_OUTPUTS "")
foreach(ts IN LISTS LMC_TS_FILES)
    get_filename_component(ts_name ${ts} NAME_WE)
    string(REGEX REPLACE "^lmc_" "" qm_name ${ts_name})
    set(qm_out ${LMC_RUNTIME_DIR}/lang/${qm_name}.qm)
    add_custom_command(
        OUTPUT ${qm_out}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_RUNTIME_DIR}/lang
        COMMAND Qt6::lrelease ${ts} -qm ${qm_out}
        DEPENDS ${ts}
        COMMENT "lrelease ${qm_name}.ts"
        VERBATIM
    )
    list(APPEND LMC_QM_OUTPUTS ${qm_out})
endforeach()

# Qt's own translations for the standard dialogs (QMessageBox buttons and the
# like): taken from the Qt used for the build and placed into lang/system next
# to the app's own lang/*.qm. The files keep Qt's qtbase_<lang>[_<COUNTRY>].qm
# naming (no renaming, nothing stored in the sources) and are matched to lmc's
# xx_YY locales at runtime (src/app/application.cpp).
# Override with -DLMC_QT_TRANSLATIONS_DIR=<abs path> for Qt builds whose
# translations live elsewhere.
if(NOT DEFINED LMC_QT_TRANSLATIONS_DIR)
    set(LMC_QT_TRANSLATIONS_DIR ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS})
endif()
if(NOT EXISTS ${LMC_QT_TRANSLATIONS_DIR}/qtbase_de.qm)
    message(FATAL_ERROR
            "Qt's own translations (qtbase_*.qm) were not found at "
            "${LMC_QT_TRANSLATIONS_DIR} (Qt ${Qt6_VERSION}). They ship with "
            "the Qt translations package (Debian/Ubuntu: "
            "qt6-translations-l10n) and with the translations component of "
            "the Qt installer / vcpkg. If your Qt build has no translations, "
            "point CMake at one that has via "
            "-DLMC_QT_TRANSLATIONS_DIR=<abs path>.")
endif()

set(LMC_QT_SYSTEM_QMS "")
# Drop stale copies (another Qt's set) so nothing stale is ever staged;
# repopulated at build time by the commands below.
file(REMOVE_RECURSE ${LMC_RUNTIME_DIR}/lang/system)
set(LMC_QT_SYSTEM_SKIPPED "")
foreach(ts IN LISTS LMC_TS_FILES)
    get_filename_component(ts_name ${ts} NAME_WE)
    string(REGEX REPLACE "^lmc_" "" locale ${ts_name})
    # en_US is the source language; Qt's qtbase_en.qm is an identity file.
    if(locale STREQUAL "en_US")
        continue()
    endif()
    # Qt ships qtbase_<lang>.qm for most languages and
    # qtbase_<lang>_<COUNTRY>.qm for country specific ones (pt_BR).
    string(REGEX REPLACE "_.*" "" qt_lang ${locale})
    set(qt_file "")
    foreach(name IN ITEMS qtbase_${locale}.qm qtbase_${qt_lang}.qm)
        if(EXISTS ${LMC_QT_TRANSLATIONS_DIR}/${name})
            set(qt_file ${name})
            break()
        endif()
    endforeach()
    if(NOT qt_file)
        list(APPEND LMC_QT_SYSTEM_SKIPPED ${locale})
        continue()
    endif()
    set(qm_dst ${LMC_RUNTIME_DIR}/lang/system/${qt_file})
    # Build-time copy (not configure_file): the outputs join lmc_runtime_files
    # below, so a deleted staging dir is repopulated without reconfiguring.
    add_custom_command(
        OUTPUT ${qm_dst}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_RUNTIME_DIR}/lang/system
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${LMC_QT_TRANSLATIONS_DIR}/${qt_file} ${qm_dst}
        DEPENDS ${LMC_QT_TRANSLATIONS_DIR}/${qt_file}
        COMMENT "Bundling ${qt_file}"
        VERBATIM
    )
    list(APPEND LMC_QT_SYSTEM_QMS ${qm_dst})
endforeach()
list(LENGTH LMC_QT_SYSTEM_QMS qt_qm_count)
message(STATUS "Bundling Qt translations from ${LMC_QT_TRANSLATIONS_DIR}: "
        "${qt_qm_count} locales into ${LMC_RUNTIME_DIR}/lang/system")
if(LMC_QT_SYSTEM_SKIPPED)
    string(REPLACE ";" " " skipped_str "${LMC_QT_SYSTEM_SKIPPED}")
    message(STATUS "Qt has no translations for: ${skipped_str} "
            "(standard dialogs stay in English for these)")
endif()

add_custom_target(lmc_runtime_files ALL
    DEPENDS
        ${LMC_RUNTIME_DIR}/lmc.rcc
        ${LMC_QM_OUTPUTS}
        ${LMC_QT_SYSTEM_QMS}
)
add_dependencies(lmc lmc_runtime_files)

# ---------------------------------------------------------------------------
# Release layout (step 1.3): the install() rules below stage the flat runtime
# tree that setup/** packages (from release in CI via LMC_RELEASE_DIR).
# Local staging: cmake --build <build-dir> --target package-local
# ---------------------------------------------------------------------------
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

# Qt imageformat plugins live in the Qt installation's plugins/ directory:
# locate it relative to the Qt6Core CMake package (covers both the standard
# <prefix>/plugins layout and Debian's multiarch qt6/plugins layout). Override
# with -DQT_PLUGINS_DIR=<path> for other installations. Windows ships none
# here (windeployqt covers them in CI).
if(NOT WIN32)
    set(QT_PLUGINS_DIR "" CACHE PATH
        "Qt plugin directory (autodetected from the Qt6Core package when empty)")
    if(NOT QT_PLUGINS_DIR)
        foreach(_plugins_cand
                ../../../plugins
                ../../plugins
                ../../../qt6/plugins
                ../../qt6/plugins
                ../../../qt5/plugins
                ../../qt5/plugins
                ../../../qt/plugins
                ../../qt/plugins)
            get_filename_component(_plugins_abs
                "${Qt6Core_DIR}/${_plugins_cand}" ABSOLUTE)
            if(EXISTS "${_plugins_abs}/imageformats")
                set(QT_PLUGINS_DIR "${_plugins_abs}")
                break()
            endif()
        endforeach()
    endif()
    if(NOT QT_PLUGINS_DIR)
        message(FATAL_ERROR
            "Could not locate the Qt plugin directory; pass -DQT_PLUGINS_DIR=<path>")
    endif()
endif()
if(APPLE)
    foreach(_plugin libqgif.dylib libqico.dylib libqjpeg.dylib)
        install(FILES ${QT_PLUGINS_DIR}/imageformats/${_plugin}
                DESTINATION LAN-Messenger.app/Contents/Plugins/imageformats)
    endforeach()
    # TIFF moved out of qtbase in Qt 6 (qtimageformats module); macOS shipped
    # it historically, so take it when present and skip silently otherwise.
    install(FILES ${QT_PLUGINS_DIR}/imageformats/libqtiff.dylib
            DESTINATION LAN-Messenger.app/Contents/Plugins/imageformats
            OPTIONAL)
elseif(UNIX)
    foreach(_plugin libqgif.so libqico.so libqjpeg.so)
        install(FILES ${QT_PLUGINS_DIR}/imageformats/${_plugin}
                DESTINATION imageformats)
    endforeach()
    # Platform plugin and Qt 6 TLS backends for standalone bundles (QSslSocket
    # needs a TLS plugin). QCoreApplication::libraryPaths() includes the
    # application directory, so plugins next to the binary are found.
    install(FILES ${QT_PLUGINS_DIR}/platforms/libqxcb.so DESTINATION platforms)
    file(GLOB _tls_plugins "${QT_PLUGINS_DIR}/tls/*")
    if(_tls_plugins)
        install(FILES ${_tls_plugins} DESTINATION tls)
    endif()
endif()

if(WIN32)
    # OpenSSL DLLs from the openssl/bin redistributable.
    file(GLOB _openssl_dlls
        ${CMAKE_SOURCE_DIR}/openssl/bin/libcrypto*.dll
        ${CMAKE_SOURCE_DIR}/openssl/bin/libssl*.dll)
    if(_openssl_dlls)
        install(FILES ${_openssl_dlls} DESTINATION .)
    endif()
endif()

if(UNIX AND NOT APPLE)
    # Launcher script (sets LD_LIBRARY_PATH) and the Unity tray whitelist helper.
    install(PROGRAMS
        scripts/lan-messenger.sh
        scripts/whitelist
        DESTINATION .
    )
endif()

# package-local: stage the complete layout in one command. LMC_RELEASE_DIR
# defaults to <build>/release; CI points it at release where setup/**
# expects the layout.
set(LMC_RELEASE_DIR "${CMAKE_BINARY_DIR}/release" CACHE PATH
    "Release layout staging directory for the package-local target")
add_custom_target(package-local
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}
            --config $<CONFIG> --prefix ${LMC_RELEASE_DIR}
    COMMENT "Staging release layout into ${LMC_RELEASE_DIR}"
    VERBATIM
)
add_dependencies(package-local lmc lmc_runtime_files)
