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

if( NOT DBUS_SYSTEM_BUS )
  set( DBUS_SYSTEM_BUS "unix:path=/var/run/dbus/system_bus_socket" CACHE INTERNAL "" FORCE )
endif()

if( WITH_UPOWER )
  add_definitions( -DWITH_UPOWER )
endif( )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/tdmlib
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES shutdown.png DESTINATION ${DATA_INSTALL_DIR}/ksmserver/pics )
install( FILES shutdownkonq.png DESTINATION ${DATA_INSTALL_DIR}/ksmserver/pics )
install( FILES ksmserver.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )
install( FILES move_session_config.sh DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )


##### ksmserver (tdeinit) #######################

tde_add_tdeinit_executable( ksmserver AUTOMOC
  SOURCES
    main.cpp server.cpp shutdowndlg.cpp
    legacy.cpp startup.cpp shutdown.cpp client.cpp
    KSMServerInterface.skel server.skel timed.ui
  LINK dmctl-static tdeui-shared tdersync-shared ${TDEHW_LIBRARIES}
)
