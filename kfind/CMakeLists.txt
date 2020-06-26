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

add_subdirectory( pics )

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

tde_install_icons( kfind )
tde_create_translated_desktop(
  SOURCE Kfind.desktop
  PO_DIR kfind-desktops
)
tde_create_translated_desktop(
  SOURCE kfindpart.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR kfind-desktops
)
tde_create_translated_desktop(
  SOURCE findpart.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR kfind-desktops
)


##### libkfindpart (module) #####################

tde_add_kpart( libkfindpart AUTOMOC
  SOURCES kfindpart.cpp
  LINK kfind_common-static konq-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### kfind (executable) ######################

tde_add_executable( kfind AUTOMOC
  SOURCES kfwin.cpp kfinddlg.cpp main.cpp
  LINK kfind_common-static
  DESTINATION ${BIN_INSTALL_DIR}
)


##### kfind_common (static) #####################

tde_add_library( kfind_common STATIC_PIC AUTOMOC
  SOURCES kftabdlg.cpp kfind.cpp kquery.cpp kdatecombo.cpp
  LINK tdeparts-shared
)
