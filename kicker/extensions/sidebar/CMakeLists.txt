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

if( NOT BUILD_LIBKONQ )
  include( "${TDE_CMAKE_DIR}/libkonq.cmake" )
endif( NOT BUILD_LIBKONQ )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE sidebarextension.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/extensions
  PO_DIR kicker-desktops
)


##### sidebar_panelextension (module) ###########

tde_add_kpart( sidebar_panelextension AUTOMOC
  SOURCES sidebarextension.cpp
  LINK konq-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
