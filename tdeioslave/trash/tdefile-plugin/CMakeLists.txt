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
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE
    tdefile_trash.desktop tdefile_trash_system.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)


##### tdefile_trash (module) ######################

set( target tdefile_trash )

tde_add_kpart( ${target} AUTOMOC
  SOURCES tdefile_trash.cpp
  LINK trashcommon-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
