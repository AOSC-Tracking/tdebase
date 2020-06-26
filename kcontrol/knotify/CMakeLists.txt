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

add_subdirectory( sounds )

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
  SOURCE kcmnotify.desktop
  PO_DIR kcontrol-desktops
)
install( FILES play.png DESTINATION ${DATA_INSTALL_DIR}/kcontrol/pics )


##### kcm_knotify (module) ######################

tde_add_kpart( kcm_knotify AUTOMOC
  SOURCES knotify.cpp playersettings.ui
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
