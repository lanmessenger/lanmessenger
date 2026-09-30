find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Network Xml LinguistTools)
if(UNIX)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Multimedia)
endif()
find_package(OpenSSL REQUIRED)

# Generated runtime files (lmc.rcc, *.qm) land here; the flat release layout
# itself is staged from the install() rules at the bottom.
set(LMC_RUNTIME_DIR ${CMAKE_CURRENT_BINARY_DIR}/runtime)
set(LMC_RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/resources)

add_executable(lmc
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
    src/main.cpp
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
    src/resource.qrc
)

target_compile_definitions(lmc PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x050F00
)

target_include_directories(lmc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Qt5::Sql is not linked (unused; see qt6-migration-plan.md).
target_link_libraries(lmc
    PRIVATE
        lmcapp
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Xml
        OpenSSL::Crypto
)

if(UNIX)
    target_link_libraries(lmc PRIVATE Qt5::Multimedia)
endif()

if(WIN32)
    # GetUserNameW(...) in Helper::getLogonName (shared.cpp)
    target_link_libraries(lmc PRIVATE advapi32)
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
    COMMAND Qt5::rcc -binary ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.qrc
            -o ${LMC_RUNTIME_DIR}/lmc.rcc
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.qrc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
    COMMENT "Building external resource bundle lmc.rcc"
    VERBATIM
)

# Translations: lang/*.qm (sysLangDir = <appdir>/lang, stdlocation.h).
set(LMC_TS_FILES
    src/ar_SA.ts
    src/bg_BG.ts
    src/de_DE.ts
    src/el_GR.ts
    src/en_US.ts
    src/es_ES.ts
    src/fr_FR.ts
    src/hu_HU.ts
    src/it_IT.ts
    src/ja_JP.ts
    src/ko_KR.ts
    src/ml_IN.ts
    src/pl_PL.ts
    src/pt_BR.ts
    src/ro_RO.ts
    src/ru_RU.ts
    src/sk_SK.ts
    src/sl_SI.ts
    src/sv_SE.ts
    src/tr_TR.ts
)

set(LMC_QM_OUTPUTS "")
foreach(ts IN LISTS LMC_TS_FILES)
    get_filename_component(qm_name ${ts} NAME_WE)
    set(qm_out ${LMC_RUNTIME_DIR}/lang/${qm_name}.qm)
    add_custom_command(
        OUTPUT ${qm_out}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_RUNTIME_DIR}/lang
        COMMAND Qt5::lrelease ${CMAKE_CURRENT_SOURCE_DIR}/${ts} -qm ${qm_out}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ts}
        COMMENT "lrelease ${qm_name}.ts"
        VERBATIM
    )
    list(APPEND LMC_QM_OUTPUTS ${qm_out})
endforeach()

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
install(DIRECTORY ${LMC_RUNTIME_DIR}/lang/ DESTINATION ${LMC_LAYOUT_BIN_DIR}/lang
        FILES_MATCHING PATTERN "*.qm")
# Qt's own translations shipped in the sources (resources/lang/system).
install(DIRECTORY ${LMC_RES_DIR}/lang/system DESTINATION ${LMC_LAYOUT_BIN_DIR}/lang)
# Themes without Classic (bundled into resource.qrc) and notification sounds.
install(DIRECTORY ${LMC_RES_DIR}/themes DESTINATION ${LMC_LAYOUT_BIN_DIR}
        PATTERN "Classic" EXCLUDE)
install(DIRECTORY ${LMC_RES_DIR}/sounds DESTINATION ${LMC_LAYOUT_BIN_DIR})
install(FILES ${LMC_RES_DIR}/text/license.txt DESTINATION ${LMC_LAYOUT_BIN_DIR}
        RENAME ${LMC_LAYOUT_LICENSE})

# Qt imageformat plugins live in the Qt installation's plugins/ directory:
# locate it relative to the Qt5Core CMake package (covers both the standard
# <prefix>/plugins layout and Debian's multiarch qt5/plugins layout). Override
# with -DQT_PLUGINS_DIR=<path> for other installations. Windows ships none
# here (windeployqt covers them in CI).
if(NOT WIN32)
    set(QT_PLUGINS_DIR "" CACHE PATH
        "Qt plugin directory (autodetected from the Qt5Core package when empty)")
    if(NOT QT_PLUGINS_DIR)
        foreach(_plugins_cand
                ../../../plugins
                ../../plugins
                ../../../qt5/plugins
                ../../qt5/plugins
                ../../../qt/plugins
                ../../qt/plugins)
            get_filename_component(_plugins_abs
                "${Qt5Core_DIR}/${_plugins_cand}" ABSOLUTE)
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
    foreach(_plugin libqgif.dylib libqico.dylib libqjpeg.dylib libqtiff.dylib)
        install(FILES ${QT_PLUGINS_DIR}/imageformats/${_plugin}
                DESTINATION LAN-Messenger.app/Contents/Plugins/imageformats)
    endforeach()
elseif(UNIX)
    foreach(_plugin libqgif.so libqico.so libqjpeg.so)
        install(FILES ${QT_PLUGINS_DIR}/imageformats/${_plugin}
                DESTINATION imageformats)
    endforeach()
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
