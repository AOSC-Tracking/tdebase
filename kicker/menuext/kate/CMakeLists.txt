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
  SOURCE katesessionmenu.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/menuext
  PO_DIR kicker-desktops
)


##### kickermenu_kate (module) ##################

tde_add_kpart( kickermenu_kate AUTOMOC
  SOURCES katesessionmenu.cpp
  LINK tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
