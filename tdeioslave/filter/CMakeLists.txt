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
  SOURCE gzip.protocol bzip.protocol bzip2.protocol xz.protocol lzma.protocol lzip.protocol
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)


##### tdeio_filter (module) #######################

set( target tdeio_filter )

tde_add_kpart( ${target} AUTOMOC
  SOURCES filter.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
