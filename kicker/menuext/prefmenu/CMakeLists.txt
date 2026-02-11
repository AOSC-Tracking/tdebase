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
  ${CMAKE_BINARY_DIR}/kicker/libkicker
  ${CMAKE_SOURCE_DIR}/kicker/libkicker
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE prefmenu.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/menuext
  PO_DIR kicker-desktops
)


##### kickermenu_prefmenu (module) ##############

tde_add_kpart( kickermenu_prefmenu AUTOMOC
  SOURCES prefmenu.cpp
  LINK kickermain-shared tdeui-shared
  DEPENDENCIES kicker+libkicker+kickerSettings.cpp
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
