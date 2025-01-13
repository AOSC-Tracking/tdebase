#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

include( ConfigureChecks.cmake )

add_subdirectory( lock )
add_subdirectory( pics )
add_subdirectory( patterns )
add_subdirectory( programs )
add_subdirectory( init )
add_subdirectory( kwebdesktop )
add_subdirectory( dbus/interfaces )
add_subdirectory( dbus/screensaver )


if( NOT BUILD_LIBKONQ )
  include( "${TDE_CMAKE_DIR}/libkonq.cmake" )
endif( NOT BUILD_LIBKONQ )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/tdmlib
  ${CMAKE_SOURCE_DIR}/kcontrol/background
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
  ${DBUS_TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${LIBART_LIBRARY_DIRS}
  ${DBUS_TQT_LIBRARY_DIRS}
)


##### headers ###################################

install(
  FILES KDesktopIface.h KScreensaverIface.h KBackgroundIface.h 
  DESTINATION ${INCLUDE_INSTALL_DIR} 
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE kdesktop.desktop.cmake
  DESTINATION ${AUTOSTART_INSTALL_DIR}
  PO_DIR kdesktop-desktops
)

install( FILES kdesktop_custom_menu1 kdesktop_custom_menu2 DESTINATION ${CONFIG_INSTALL_DIR} )

tde_create_translated_desktop(
  SOURCE kdesktopSetAsBackground.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqueror/servicemenus
  PO_DIR kdesktop-desktops
)

install( FILES kdesktop.kcfg tdelaunch.kcfg DESTINATION ${KCFG_INSTALL_DIR} )


##### kcheckrunning (executable) ################

tde_add_executable( kcheckrunning
  SOURCES kcheckrunning.cpp
  LINK ${TQT_LIBRARIES} X11
  DESTINATION ${BIN_INSTALL_DIR}
)


##### kdesktop (tdeinit) ########################

set( target kdesktop )
set_property( SOURCE lockeng.cpp APPEND PROPERTY COMPILE_DEFINITIONS KDE_CONFDIR="${TDE_CONFIG_DIR}" )

set( ${target}_SRCS
  main.cpp krootwm.cpp xautolock.cpp kdiconview.cpp init.cpp
  desktop.cpp lockeng.cpp KDesktopIface.skel bgmanager.cpp
  KScreensaverIface.skel minicli.cpp KBackgroundIface.skel
  pixmapserver.cpp kcustommenu.cpp startupid.cpp minicli_ui.ui
  xautolock_diy.c xautolock_engine.c kshadowengine.cpp
  kshadowsettings.cpp kdesktopshadowsettings.cpp
  tdefileividesktop.cpp kdesktopapp.cpp
)

tde_add_tdeinit_executable( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK
    pthread kdesktopsettings-static bgnd-static dmctl-static
    dbusscreensaverservice-static dbusscreensaveriface-static
    konq-shared tdeutils-shared tdesu-shared ${TDEHW_LIBRARIES}
    ${XRENDER_LIBRARIES} ${XCURSOR_LIBRARIES} Xext ${CMAKE_DL_LIBS}
    ${XSS_LIBRARIES} ${DBUS_1_TQT_LIBRARIES}
)

add_dependencies(${target} dbusscreensaveriface-static dbusscreensaverservice-static )


##### kdesktopsettings (static) #################

tde_add_library( kdesktopsettings STATIC_PIC
  SOURCES kdesktopsettings.kcfgc tdelaunchsettings.kcfgc
)
