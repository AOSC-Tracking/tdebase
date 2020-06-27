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

add_subdirectory( libmediacommon )
add_subdirectory( mediamanager )
add_subdirectory( medianotifier )
add_subdirectory( mounthelper )
add_subdirectory( tdefile-plugin )
add_subdirectory( tdecmodule )
add_subdirectory( mimetypes )
add_subdirectory( services )
add_subdirectory( propsdlgplugin )


include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE media.protocol
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)


##### tdeiomedia (static) #########################

set( target tdeiomedia )

tde_add_library( ${target} STATIC_PIC AUTOMOC
  SOURCES tdeio_media.cpp mediaimpl.cpp mediaimpl.skel
)


##### tdeio_media (module) ########################

set( target tdeio_media )

tde_add_kpart( ${target} AUTOMOC
  SOURCES dummy.cpp
  EMBED tdeiomedia-static
  LINK mediacommon-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
