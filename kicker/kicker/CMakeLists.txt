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

if( NOT BUILD_LIBKONQ )
  include( "${TDE_CMAKE_DIR}/libkonq.cmake" )
endif( NOT BUILD_LIBKONQ )

add_subdirectory( core )
add_subdirectory( interfaces )
add_subdirectory( ui )
add_subdirectory( buttons )

include_directories(
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS} )

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${DBUS_TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE panel.desktop
  DESTINATION ${AUTOSTART_INSTALL_DIR}
  PO_DIR kicker-desktops
)
tde_create_translated_desktop(
  SOURCE kcmkicker.desktop
  PO_DIR kicker-desktops
)
install( FILES kickerrc.upd DESTINATION ${DATA_INSTALL_DIR}/tdeconf_update )

install( PROGRAMS
    kicker-3.1-properSizeSetting.pl kicker-3.5-taskbarEnums.pl
  DESTINATION ${DATA_INSTALL_DIR}/tdeconf_update )


##### kicker-3.4-reverseLayout (executable) #####

tde_add_executable( kicker-3.4-reverseLayout
  SOURCES kicker-3.4-reverseLayout.cpp
  LINK tdecore-shared
  DESTINATION ${LIB_INSTALL_DIR}/tdeconf_update_bin
)


##### kicker (tdeinit) ##########################

configure_file( ${TDE_CMAKE_TEMPLATES}/tde_dummy_cpp.cmake dummy.cpp COPYONLY )

tde_add_tdeinit_executable( kicker
  SOURCES dummy.cpp
  EMBED kicker_core-static
  LINK kicker_buttons-static kicker_ui-static kickermain-shared tdeabc-shared tdeutils-shared
)
