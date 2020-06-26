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

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/konqueror/sidebar
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TDE_LIBRARY_DIRS}
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE webmodule_add.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/add
  PO_DIR konqueror-desktops
)
install( FILES websidebar.html DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/websidebar )


##### konqsidebar_web (module) ##################

set( target konqsidebar_web )

tde_add_kpart( ${target} AUTOMOC
  SOURCES web_module.cpp
  LINK konqsidebarplugin-shared konq-shared tdehtml-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
