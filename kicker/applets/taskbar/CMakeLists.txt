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
  ${CMAKE_SOURCE_DIR}/kicker/taskbar
  ${CMAKE_SOURCE_DIR}/kicker/taskmanager
  ${CMAKE_SOURCE_DIR}/kicker/libkicker
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE taskbarapplet.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/applets
  PO_DIR kicker-desktops
)


##### taskbar_panelapplet (module) ##############

tde_add_kpart( taskbar_panelapplet AUTOMOC
  SOURCES taskbarapplet.cpp
  LINK taskbar-shared kickermain-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
