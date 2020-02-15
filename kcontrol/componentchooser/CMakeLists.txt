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

add_subdirectory( componentservices )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

install( FILES componentchooser.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )


##### kcm_componentchooser (module) #############

tde_add_kpart( kcm_componentchooser AUTOMOC
  SOURCES
    componentchooser_ui.ui componentconfig_ui.ui emailclientconfig_ui.ui terminalemulatorconfig_ui.ui
    browserconfig_ui.ui filemanagerconfig_ui.ui componentchooser.cpp kcm_componentchooser.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
