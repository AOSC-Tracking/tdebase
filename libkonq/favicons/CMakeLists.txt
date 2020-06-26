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
  ${TDE_LIBRARY_DIRS}
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE favicons.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}/kded
  PO_DIR libkonq-desktops
)
install( FILES favicons.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )
install( PROGRAMS move_favicons.sh DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )


##### kded_favicons (module) ####################

tde_add_kpart( kded_favicons AUTOMOC
  SOURCES favicons.cpp favicons.skel
  LINK tdeinit_kded-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
