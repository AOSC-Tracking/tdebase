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
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/twin/lib
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE twindecoration.desktop
  PO_DIR twin-desktops
)


##### kcm_twindecoration (module) ###############

tde_add_kpart( kcm_twindecoration AUTOMOC
  SOURCES
    twindecoration.cpp buttons.cpp
    twindecorationIface.skel preview.cpp
  LINK tdecorations-shared tdeui-shared Xext
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
