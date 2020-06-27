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

check_include_file( "sys/statvfs.h" HAVE_STATVFS )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/tdeioslave/media/libmediacommon
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE tdefile_media.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)


##### tdefile_media (module) ######################

set( target tdefile_media )

tde_add_kpart( ${target} AUTOMOC
  SOURCES tdefilemediaplugin.cpp
  LINK mediacommon-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
