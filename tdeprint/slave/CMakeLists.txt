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

add_definitions( -D_TDEPRINT_COMPILE )

add_subdirectory( mimetypes )
add_subdirectory( templates )

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
  SOURCE print.protocol printdb.protocol
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeprint-desktops
)


##### tdeio_print (module) ########################

tde_add_kpart( tdeio_print AUTOMOC
  SOURCES tdeio_print.cpp
  LINK tdeprint-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
