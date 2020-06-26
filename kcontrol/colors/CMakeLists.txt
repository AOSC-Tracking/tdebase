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
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE colors.desktop
  PO_DIR kcontrol-desktops
)


##### kcm_colors (module) #######################

tde_add_kpart( kcm_colors AUTOMOC
  SOURCES colorscm.cpp widgetcanvas.cpp
  LINK krdb-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
