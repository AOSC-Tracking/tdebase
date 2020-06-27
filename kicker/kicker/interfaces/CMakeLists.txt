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


##### headers ###################################

install( FILES
    kickoff-search-plugin.h kickoffsearchinterface.h
  DESTINATION ${INCLUDE_INSTALL_DIR} )


##### other data ################################

tde_create_translated_desktop(
  SOURCE kickoffsearchplugin.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR kicker-desktops
)


##### kickoffsearch_interfaces (shared) #########

set( target kickoffsearch_interfaces )

set( ${target}_SRCS
  kickoff-search-plugin.cpp kickoffsearchinterface.cpp
)

tde_add_library( ${target} SHARED AUTOMOC
  SOURCES ${${target}_SRCS}
  VERSION 0.0.0
  LINK ${TQT_LIBRARIES}
  DESTINATION ${LIB_INSTALL_DIR}
)
