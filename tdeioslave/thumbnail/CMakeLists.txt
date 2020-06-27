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

##### check for openexr #########################

if( WITH_OPENEXR )
  pkg_search_module( OPENEXR OpenEXR )
  if( NOT OPENEXR_FOUND )
    message(FATAL_ERROR "\nopenexr are requested, but not found on your system" )
  endif( NOT OPENEXR_FOUND )
  set( HAVE_EXR 1 )
endif( WITH_OPENEXR )


##### compiler/linker flags #####################

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${XCURSOR_INCLUDE_DIRS}
  ${OPENEXR_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE
    thumbnail.protocol imagethumbnail.desktop textthumbnail.desktop
    htmlthumbnail.desktop djvuthumbnail.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)

tde_create_translated_desktop(
  SOURCE thumbcreator.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)


##### tdeio_thumbnail (module) ####################

set( target tdeio_thumbnail )

tde_add_kpart( ${target} AUTOMOC
  SOURCES thumbnail.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### imagethumbnail (module) ####################

set( target imagethumbnail )

tde_add_kpart( ${target} AUTOMOC
  SOURCES imagecreator.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### textthumbnail (module) ####################

set( target textthumbnail )

tde_add_kpart( ${target} AUTOMOC
  SOURCES textcreator.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### htmlthumbnail (module) ####################

set( target htmlthumbnail )

tde_add_kpart( ${target} AUTOMOC
  SOURCES htmlcreator.cpp
  LINK tdehtml-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### djvuthumbnail (module) ####################

set( target djvuthumbnail )

tde_add_kpart( ${target} AUTOMOC
  SOURCES djvucreator.cpp
  LINK tdecore-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### cursorthumbnail (module) ##################

if( WITH_XCURSOR )

  set( target cursorthumbnail )

  tde_add_kpart( ${target} AUTOMOC
    SOURCES cursorcreator.cpp
    LINK tdecore-shared ${XCURSOR_LIBRARIES}
    DESTINATION ${PLUGIN_INSTALL_DIR}
  )

  tde_create_translated_desktop(
    SOURCE cursorthumbnail.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}
    PO_DIR tdeioslave-desktops
  )

endif( WITH_XCURSOR )


##### exrthumbnail (module) #####################

if( WITH_OPENEXR )

  set( target exrthumbnail )

  tde_add_kpart( ${target} AUTOMOC
    SOURCES exrcreator.cpp
    LINK tdecore-shared ${OPENEXR_LIBRARIES}
    DESTINATION ${PLUGIN_INSTALL_DIR}
  )

  tde_create_translated_desktop(
    SOURCE exrthumbnail.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}
    PO_DIR tdeioslave-desktops
  )

endif( WITH_OPENEXR )
