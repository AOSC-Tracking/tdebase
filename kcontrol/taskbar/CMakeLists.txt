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

if( NOT BUILD_KICKER )
  include( "${TDE_CMAKE_DIR}/kicker.cmake" )
endif( NOT BUILD_KICKER )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### tdeconf_update ############################

add_subdirectory( tdeconf_update )


##### other data ################################

tde_create_translated_desktop(
  SOURCE kcmtaskbar.desktop
  PO_DIR kcontrol-desktops
)
install( FILES uninstall.desktop DESTINATION ${APPS_INSTALL_DIR}/Settings/LookNFeel RENAME kcmtaskbar.desktop )


##### kcm_taskbar (module) ######################

add_custom_command( OUTPUT taskbarsettings.h
  COMMAND ${KDE3_KCFGC_EXECUTABLE}
    ${CMAKE_SOURCE_DIR}/kicker/taskbar/taskbar.kcfg
    ${CMAKE_SOURCE_DIR}/kicker/taskbar/taskbarsettings.kcfgc
  DEPENDS
    ${CMAKE_SOURCE_DIR}/kicker/taskbar/taskbar.kcfg
    ${CMAKE_SOURCE_DIR}/kicker/taskbar/taskbarsettings.kcfgc )

set_source_files_properties( kcmtaskbar.cpp
  PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/taskbarsettings.h )

tde_add_kpart( kcm_taskbar AUTOMOC
  SOURCES kcmtaskbarui.ui kcmtaskbar.cpp
  LINK taskbar-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
