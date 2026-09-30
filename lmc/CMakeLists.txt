find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Network Xml LinguistTools)
if(UNIX)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Multimedia)
endif()
find_package(OpenSSL REQUIRED)

# Release layout: binary and runtime files end up here (like qmake
# DESTDIR = ../release); step 1.3 completes the layout (themes/sounds/plugins).
set(LMC_RELEASE_DIR ${CMAKE_BINARY_DIR}/release)

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

# All configurations write into a single release dir: the custom commands
# below know nothing about multi-config <CONFIG> subdirectories.
set_target_properties(lmc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${LMC_RELEASE_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${LMC_RELEASE_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${LMC_RELEASE_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LMC_RELEASE_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${LMC_RELEASE_DIR}
)

# External resource bundle (2nd role of resource.qrc): looked up at runtime as
# <cwd>/lmc.rcc (StdLocation::resourceFile(), stdlocation.h) and registered via
# QResource::registerResource (main.cpp). The 1st role — embedding into the
# binary — is done by AUTORCC (src/resource.qrc in the target sources).
add_custom_command(
    OUTPUT ${LMC_RELEASE_DIR}/lmc.rcc
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_RELEASE_DIR}
    COMMAND Qt5::rcc -binary ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.qrc
            -o ${LMC_RELEASE_DIR}/lmc.rcc
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.qrc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
    COMMENT "Building external resource bundle lmc.rcc"
    VERBATIM
)

# Translations: lang/*.qm (sysLangDir = <cwd>/lang, stdlocation.h) plus
# lang/system — a copy of resources/lang/system (Qt's own translations).
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
    set(qm_out ${LMC_RELEASE_DIR}/lang/${qm_name}.qm)
    add_custom_command(
        OUTPUT ${qm_out}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_RELEASE_DIR}/lang
        COMMAND Qt5::lrelease ${CMAKE_CURRENT_SOURCE_DIR}/${ts} -qm ${qm_out}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ts}
        COMMENT "lrelease ${qm_name}.ts"
        VERBATIM
    )
    list(APPEND LMC_QM_OUTPUTS ${qm_out})
endforeach()

file(GLOB LMC_SYSTEM_QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/lang/system/*.qm)
add_custom_command(
    OUTPUT ${LMC_RELEASE_DIR}/lang/system.stamp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LMC_RELEASE_DIR}/lang
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/lang/system
            ${LMC_RELEASE_DIR}/lang/system
    COMMAND ${CMAKE_COMMAND} -E touch ${LMC_RELEASE_DIR}/lang/system.stamp
    DEPENDS ${LMC_SYSTEM_QM_FILES}
    COMMENT "Copying system Qt translations to lang/system"
    VERBATIM
)

add_custom_target(lmc_runtime_files ALL
    DEPENDS
        ${LMC_RELEASE_DIR}/lmc.rcc
        ${LMC_RELEASE_DIR}/lang/system.stamp
        ${LMC_QM_OUTPUTS}
)
add_dependencies(lmc lmc_runtime_files)
