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

add_subdirectory( kdedmodule )
add_subdirectory( entries )
add_subdirectory( mimetypes )

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
  SOURCE system.protocol
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)


##### tdeiosystem (static) ########################

set( target tdeiosystem )

tde_add_library( ${target} STATIC_PIC AUTOMOC
  SOURCES tdeio_system.cpp systemimpl.cpp
)


##### tdeio_system (module) #######################

set( target tdeio_system )

tde_add_kpart( ${target} AUTOMOC
  SOURCES dummy.cpp
  EMBED tdeiosystem-static
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
