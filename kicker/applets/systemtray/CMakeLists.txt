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
  ${CMAKE_BINARY_DIR}/kicker/applets/clock
  ${CMAKE_SOURCE_DIR}/kicker/applets/clock
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${CMAKE_BINARY_DIR}/kicker/applets/clock
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE systemtrayapplet.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/applets
  PO_DIR kicker-desktops
)


##### systemtray_panelapplet (module) ###########

set( target systemtray_panelapplet )

set( ${target}_SRCS
  systemtrayapplet.cpp systemtrayapplet.skel
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK kickermain-shared clock_panelapplet-static
  DEPENDENCIES kicker_core-static clock_panelapplet-static kicker+libkicker+kickerSettings.cpp
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
