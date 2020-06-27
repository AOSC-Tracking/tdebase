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

add_subdirectory( keramik )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE style.desktop
  PO_DIR kcontrol-desktops
)


##### kcm_style (module) ########################

tde_add_kpart( kcm_style AUTOMOC
  SOURCES
    kcmstyle.cpp stylepreview.ui menupreview.cpp
    styleconfdialog.cpp
  LINK krdb-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
