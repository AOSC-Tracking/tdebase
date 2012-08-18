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

if( WITH_ARTS )
  pkg_search_module( ARTS arts )
  if( NOT ARTS_FOUND )
    tde_message_fatal( "aRts are requested, but not found on your system" )
  endif( )
  set( HAVE_ARTS 1 CACHE INTERNAL "" FORCE )
endif( )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/khotkeys/kcontrol
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/khotkeys/shared
  ${CMAKE_SOURCE_DIR}/khotkeys/kcontrol
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${ARTS_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${ARTS_LIBRARY_DIRS}
)


##### khotkeys_arts (module) #######################

tde_add_kpart( khotkeys_arts AUTOMOC
  SOURCES soundrecorder_arts.cpp voicerecorder_arts.cpp
  LINK khotkeys_shared-shared artskde-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
  DEPENDENCIES ui-static
)
