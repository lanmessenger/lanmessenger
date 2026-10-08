find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network Xml LinguistTools)
if(UNIX)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Multimedia)
endif()
find_package(OpenSSL REQUIRED)

# Generated runtime files (lmc.rcc, *.qm) land here; the flat release layout
# itself is staged from the install() rules at the bottom.
set(LMC_RUNTIME_DIR ${CMAKE_CURRENT_BINARY_DIR}/runtime)
set(LMC_RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/resources)

add_library(lmc_core STATIC
    src/aboutdialog.cpp
    src/broadcastwindow.cpp
    src/chathelper.cpp
    src/chatroomwindow.cpp
    src/chatwindow.cpp
    src/crypto.cpp
    src/datagram.cpp
    src/filemessagingproc.cpp
    src/filemodelview.cpp
    src/helpwindow.cpp
    src/history.cpp
    src/historywindow.cpp
    src/imagepicker.cpp
    src/imagepickeraction.cpp
    src/lmc.cpp
    src/lmcstrings.cpp
    src/mainwindow.cpp
    src/message.cpp
    src/messagelog.cpp
    src/messaging.cpp
    src/messagingproc.cpp
    src/netstreamer.cpp
    src/network.cpp
    src/qmessagebrowser.cpp
    src/settings.cpp
    src/settingsdialog.cpp
    src/shared.cpp
    src/soundplayer.cpp
    src/subcontrols.cpp
    src/tcpnetwork.cpp
    src/theme.cpp
    src/trace.cpp
    src/transferlistview.cpp
    src/transferwindow.cpp
    src/udpnetwork.cpp
    src/updatewindow.cpp
    src/userinfowindow.cpp
    src/userselectdialog.cpp
    src/usertreewidget.cpp
    src/webnetwork.cpp
    src/xmlmessage.cpp
    src/aboutdialog.h
    src/broadcastwindow.h
    src/chatdefinitions.h
    src/chatwindow.h
    src/chathelper.h
    src/chatroomwindow.h
    src/crypto.h
    src/datagram.h
    src/definitions.h
    src/filemodelview.h
    src/helpwindow.h
    src/history.h
    src/historytreewidget.h
    src/historywindow.h
    src/imagepicker.h
    src/imagepickeraction.h
    src/lmc.h
    src/lmcstrings.h
    src/mainwindow.h
    src/message.h
    src/messagelog.h
    src/messaging.h
    src/netstreamer.h
    src/network.h
    src/qmessagebrowser.h
    src/resource.h
    src/settings.h
    src/settingsdialog.h
    src/shared.h
    src/soundplayer.h
    src/stdlocation.h
    src/subcontrols.h
    src/tcpnetwork.h
    src/theme.h
    src/trace.h
    src/transferlistview.h
    src/transferwindow.h
    src/uidefinitions.h
    src/udpnetwork.h
    src/updatewindow.h
    src/userinfowindow.h
    src/userselectdialog.h
    src/usertreewidget.h
    src/webnetwork.h
    src/xmlmessage.h
    src/aboutdialog.ui
    src/broadcastwindow.ui
    src/chatroomwindow.ui
    src/chatwindow.ui
    src/helpwindow.ui
    src/historywindow.ui
    src/mainwindow.ui
    src/settingsdialog.ui
    src/transferwindow.ui
    src/updatewindow.ui
    src/userinfowindow.ui
    src/userselectdialog.ui
)

target_compile_definitions(lmc_core PUBLIC
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
    "IDA_VERSION=\"${LMC_VERSION}\""
)

# AUTOUIC puts ui_*.h in <AUTOGEN_BUILD_DIR>/include with single-config
# generators and in <AUTOGEN_BUILD_DIR>/include_<CONFIG> with multi-config
# generators (e.g. Visual Studio); expose both so consumers of lmc_core
# (lmc, tst_lmc_gui) find them regardless of the generator.
target_include_directories(lmc_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/lmc_core_autogen/include
    ${CMAKE_CURRENT_BINARY_DIR}/lmc_core_autogen/include_$<CONFIG>
)

# Qt6::Sql is not linked (unused; see qt6-migration-plan.md).
target_link_libraries(lmc_core
    PUBLIC
        lmcapp
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
# (AUTORCC over src/resource.qrc); test binaries link lmc_core and repeat the
# same .qrc so the Qt resource initializers live in an object that is always
# linked (a static lib would drop an unreferenced qrc object).
add_executable(lmc WIN32
    src/main.cpp
    src/resource.qrc
)
target_link_libraries(lmc PRIVATE lmc_core)

if(WIN32)
    target_sources(lmc PRIVATE src/lmcwin32.rc)
    set_target_properties(lmc PROPERTIES OUTPUT_NAME lmc)
elseif(APPLE)
    target_sources(lmc PRIVATE src/lmc.icns)
    set_source_files_properties(src/lmc.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
    set_target_properties(lmc PROPERTIES
        OUTPUT_NAME "LAN-Messenger"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE lmc.icns
        MACOSX_BUNDLE_BUNDLE_NAME "LAN-Messenger"
    )
else()
    set_target_properties(lmc PROPERTIES OUTPUT_NAME lan-messenger)
endif()

# External resource bundle (2nd role of resource.qrc): looked up at runtime as
# <appdir>/lmc.rcc (main sets cwd to applicationDirPath; StdLocation::resourceFile())
# and registered via QResource::registerResource (main.cpp). The 1st role —
# embedding into the binary — is done by AUTORCC (src/resource.qrc in the sources).
add_custom_command(
    OUTPUT ${LMC_RUNTIME_DIR}/lmc.rcc
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_RUNTIME_DIR}
    COMMAND Qt6::rcc -binary ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.qrc
            -o ${LMC_RUNTIME_DIR}/lmc.rcc
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.qrc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
    COMMENT "Building external resource bundle lmc.rcc"
    VERBATIM
)

# Translations: sources live in <repo>/translations/lmc_<locale>.ts (Qt Linguist
# format, target-prefixed names; adding a file there is enough, this glob picks
# it up). The lrelease output keeps the runtime layout: lang/*.qm
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
# xx_YY locales at runtime (lmcapp/src/application.cpp).
set(LMC_QT_TRANSLATIONS_DIR ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS})
if(NOT EXISTS ${LMC_QT_TRANSLATIONS_DIR}/qtbase_de.qm)
    message(FATAL_ERROR
            "Qt's own translations (qtbase_*.qm) were not found at "
            "${LMC_QT_TRANSLATIONS_DIR}. They ship with the Qt translations "
            "package (Debian/Ubuntu: qt6-translations-l10n) and with the "
            "translations component of the Qt installer / vcpkg.")
endif()

set(LMC_QT_SYSTEM_QMS "")
# Drop stale copies (another Qt's set) so nothing stale is ever staged;
# repopulated below.
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
    configure_file(${LMC_QT_TRANSLATIONS_DIR}/${qt_file} ${qm_dst} COPYONLY)
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
)
add_dependencies(lmc lmc_runtime_files)

# ---------------------------------------------------------------------------
# Release layout (step 1.3): the install() rules below stage the flat runtime
# tree that lmc/setup/** packages (from lmc/release in CI via LMC_RELEASE_DIR).
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

# lmcapp runtime library next to the binary: ship the versioned SONAME file
# (the liblmcapp.so.2 / liblmcapp.2.dylib symlink name).
if(WIN32)
    install(FILES $<TARGET_FILE:lmcapp> DESTINATION ${LMC_LAYOUT_BIN_DIR})
elseif(APPLE)
    install(FILES $<TARGET_FILE:lmcapp> DESTINATION ${LMC_LAYOUT_BIN_DIR}
            RENAME liblmcapp.2.dylib)
else()
    install(FILES $<TARGET_FILE:lmcapp> DESTINATION ${LMC_LAYOUT_BIN_DIR}
            RENAME liblmcapp.so.2)
endif()

install(FILES ${LMC_RUNTIME_DIR}/lmc.rcc DESTINATION ${LMC_LAYOUT_BIN_DIR})
# The app's own lang/*.qm (lrelease) plus Qt's lang/system/*.qm (copied at
# configure time from the Qt used for the build).
install(DIRECTORY ${LMC_RUNTIME_DIR}/lang/ DESTINATION ${LMC_LAYOUT_BIN_DIR}/lang
        FILES_MATCHING PATTERN "*.qm")
# Themes without Classic (bundled into resource.qrc) and notification sounds.
install(DIRECTORY ${LMC_RES_DIR}/themes DESTINATION ${LMC_LAYOUT_BIN_DIR}
        PATTERN "Classic" EXCLUDE)
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
        src/scripts/lan-messenger.sh
        src/scripts/whitelist
        DESTINATION .
    )
endif()

# package-local: stage the complete layout in one command. LMC_RELEASE_DIR
# defaults to <build>/release; CI points it at lmc/release where lmc/setup/**
# expects the layout.
set(LMC_RELEASE_DIR "${CMAKE_BINARY_DIR}/release" CACHE PATH
    "Release layout staging directory for the package-local target")
add_custom_target(package-local
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}
            --config $<CONFIG> --prefix ${LMC_RELEASE_DIR}
    COMMENT "Staging release layout into ${LMC_RELEASE_DIR}"
    VERBATIM
)
add_dependencies(package-local lmc lmcapp lmc_runtime_files)
