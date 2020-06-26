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
  SOURCE info.protocol
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)

install( FILES kde-info2html.conf DESTINATION ${DATA_INSTALL_DIR}/tdeio_info )
install( PROGRAMS kde-info2html DESTINATION ${DATA_INSTALL_DIR}/tdeio_info )


##### tdeio_info (module) #########################

set( target tdeio_info )

tde_add_kpart( ${target} AUTOMOC
  SOURCES info.cc
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)

