#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  Improvements and feedback are welcome
#
#################################################

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/kicker/libkicker
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/kicker/libkicker
  ${CMAKE_SOURCE_DIR}/kicker/taskmanager
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES taskbar.kcfg DESTINATION ${KCFG_INSTALL_DIR} )


##### taskbar (shared) ##########################

set( target taskbar )

set( ${target}_SRCS
  taskbarsettings.kcfgc taskcontainer.cpp taskbar.cpp
  taskbarcontainer.cpp taskbarcontainer.skel
)

tde_add_library( ${target} SHARED AUTOMOC
  SOURCES ${${target}_SRCS}
  VERSION 1.2.0
  LINK kickermain-shared taskmanager-shared
  DEPENDENCIES kicker+libkicker+kickerSettings.cpp
  DESTINATION ${LIB_INSTALL_DIR}
)
