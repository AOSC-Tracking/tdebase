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
  ${DBUS_1_TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${LIBART_LIBRARY_DIRS}
  ${DBUS_1_TQT_LIBRARY_DIRS}
)


##### headers ###################################

install( FILES
    KDesktopIface.h KScreensaverIface.h KBackgroundIface.h
  DESTINATION ${INCLUDE_INSTALL_DIR} )


##### other data ################################

configure_file( kdesktop.desktop.cmake kdesktop.desktop @ONLY )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/kdesktop.desktop DESTINATION ${AUTOSTART_INSTALL_DIR} )
install( FILES kdesktop_custom_menu1 kdesktop_custom_menu2 DESTINATION ${CONFIG_INSTALL_DIR} )
install( FILES kdesktopSetAsBackground.desktop DESTINATION ${DATA_INSTALL_DIR}/konqueror/servicemenus )
install( FILES kdesktop.kcfg tdelaunch.kcfg DESTINATION ${KCFG_INSTALL_DIR} )


##### kcheckrunning (executable) ################

tde_add_executable( kcheckrunning
  SOURCES kcheckrunning.cpp
  LINK ${TQT_LIBRARIES} X11
  DESTINATION ${BIN_INSTALL_DIR}
)


##### kxdglauncher (executable) #################

tde_add_executable( kxdglauncher
  SOURCES kxdglauncher.cpp
  LINK tdeutils-shared
  DESTINATION ${BIN_INSTALL_DIR}
)


##### kdesktop (tdeinit) ########################

set( target kdesktop )

set( ${target}_SRCS
  main.cc krootwm.cc xautolock.cc kdiconview.cc init.cc
  desktop.cc lockeng.cc KDesktopIface.skel bgmanager.cc
  KScreensaverIface.skel minicli.cpp KBackgroundIface.skel
  pixmapserver.cc kcustommenu.cc startupid.cpp minicli_ui.ui
  xautolock_diy.c xautolock_engine.c kshadowengine.cpp
  kshadowsettings.cpp kdesktopshadowsettings.cpp
  tdefileividesktop.cpp kdesktopapp.cpp
)

tde_add_tdeinit_executable( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK
    pthread kdesktopsettings-static bgnd-static dmctl-static
    konq-shared tdeutils-shared
    ${XRENDER_LIBRARIES} ${XCURSOR_LIBRARIES} Xext ${DL_LIBRARIES}
    ${XSS_LIBRARIES} ${DBUS_1_TQT_LIBRARIES}
)


##### kdesktopsettings (static) #################

tde_add_library( kdesktopsettings STATIC_PIC
  SOURCES kdesktopsettings.kcfgc tdelaunchsettings.kcfgc
)
