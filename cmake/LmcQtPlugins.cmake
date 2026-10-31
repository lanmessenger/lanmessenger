# LmcQtPlugins.cmake — shared Qt plugin location helpers.
#
# Included from src/ (release payload install rules, via LmcRelease.cmake)
# and from the top level (teststage target, via LmcTeststage.cmake). This is the
# single source of truth for QT_PLUGINS_DIR discovery and for the per-platform
# plugin file names, so the clean release tree and the hermetic teststage
# cannot drift apart.
#
# Usage (call AFTER include(), in the consumer's directory scope):
#   include(LmcQtPlugins)
#   lmc_resolve_qt_plugins()
#
# Provides:
#   QT_PLUGINS_DIR (CACHE) — resolved via qtpaths/qmake when empty; an
#     explicit -DQT_PLUGINS_DIR=<path> is always respected.
#   LMC_QT_IMAGEFORMAT_PLUGINS — full paths of the bundled imageformat plugins
#     (macOS .dylib trio / Linux .so trio; empty where a deploy tool owns them).
#   LMC_QT_PLATFORM_PLUGIN — full path of the Linux xcb platform plugin
#     (empty on other platforms).
#   LMC_QT_TLS_PLUGINS — configure-time glob of QT_PLUGINS_DIR/tls/*
#     (Linux hermetic stage; empty elsewhere).
#   LMC_QT_SQLITE_DRIVER — expected full path of the QSQLite driver.
#
# The function never warns or fatals itself; each consumer decides the
# severity (the clean Linux release stages without plugins, the macOS/Windows
# bundles and the Linux teststage require them).

set(QT_PLUGINS_DIR "" CACHE PATH
    "Qt plugin directory (queried from qtpaths/qmake when empty)")

function(lmc_resolve_qt_plugins)
  if(NOT QT_PLUGINS_DIR)
    set(_lmc_dir "")
    find_program(_lmc_qtpaths NAMES qtpaths6 qtpaths)
    if(_lmc_qtpaths)
      execute_process(COMMAND "${_lmc_qtpaths}" --plugin-dir
          OUTPUT_VARIABLE _lmc_dir
          OUTPUT_STRIP_TRAILING_WHITESPACE
          ERROR_QUIET)
    endif()
    if(NOT _lmc_dir OR NOT EXISTS "${_lmc_dir}")
      find_program(_lmc_qmake NAMES qmake6 qmake)
      if(_lmc_qmake)
        execute_process(COMMAND "${_lmc_qmake}" -query QT_INSTALL_PLUGINS
            OUTPUT_VARIABLE _lmc_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
      endif()
    endif()
    if(_lmc_dir AND EXISTS "${_lmc_dir}")
      set(QT_PLUGINS_DIR "${_lmc_dir}" CACHE PATH
          "Qt plugin directory (queried from qtpaths/qmake when empty)" FORCE)
    endif()
  endif()

  if(APPLE)
    set(_lmc_imageformats
        "${QT_PLUGINS_DIR}/imageformats/libqgif.dylib"
        "${QT_PLUGINS_DIR}/imageformats/libqico.dylib"
        "${QT_PLUGINS_DIR}/imageformats/libqjpeg.dylib")
    set(_lmc_platform "")
    set(_lmc_sqlite "${QT_PLUGINS_DIR}/sqldrivers/libqsqlite.dylib")
  elseif(WIN32)
    set(_lmc_imageformats "")
    set(_lmc_platform "")
    set(_lmc_sqlite "${QT_PLUGINS_DIR}/sqldrivers/qsqlite.dll")
  else()
    set(_lmc_imageformats
        "${QT_PLUGINS_DIR}/imageformats/libqgif.so"
        "${QT_PLUGINS_DIR}/imageformats/libqico.so"
        "${QT_PLUGINS_DIR}/imageformats/libqjpeg.so")
    set(_lmc_platform "${QT_PLUGINS_DIR}/platforms/libqxcb.so")
    set(_lmc_sqlite "${QT_PLUGINS_DIR}/sqldrivers/libqsqlite.so")
  endif()

  # TLS backends globbed at configure time (the same set the release used to
  # bundle before the clean-payload inversion).
  if(UNIX AND NOT APPLE)
    file(GLOB _lmc_tls "${QT_PLUGINS_DIR}/tls/*")
  else()
    set(_lmc_tls "")
  endif()

  set(LMC_QT_IMAGEFORMAT_PLUGINS "${_lmc_imageformats}" PARENT_SCOPE)
  set(LMC_QT_PLATFORM_PLUGIN "${_lmc_platform}" PARENT_SCOPE)
  set(LMC_QT_TLS_PLUGINS "${_lmc_tls}" PARENT_SCOPE)
  set(LMC_QT_SQLITE_DRIVER "${_lmc_sqlite}" PARENT_SCOPE)
endfunction()
