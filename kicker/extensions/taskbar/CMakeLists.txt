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
  ${CMAKE_SOURCE_DIR}/kicker/taskbar
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE taskbarextension.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/extensions
  PO_DIR kicker-desktops
)


##### taskbar_panelextension (module) ###########

set( target taskbar_panelextension )

set( ${target}_SRCS
  taskbarextension.cpp taskbarextension.skel
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK kickermain-shared taskmanager-shared taskbar-shared
  DEPENDENCIES kicker+libkicker+kickerSettings.cpp
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
