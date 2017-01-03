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

##### check for openssl #########################

option( WITH_SSL "Enable support for SSL" ON )

if( WITH_SSL )
  pkg_search_module( SSL openssl )
  if( NOT SSL_FOUND )
    tde_message_fatal( "SSL support are requested, but openssl is not found on your system" )
  endif( NOT SSL_FOUND )
  set( HAVE_SSL 1 CACHE INTERNAL "" FORCE )
endif( WITH_SSL )


##### compiler/linker settings ##################

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

install( FILES crypto.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )


##### kcm_crypto (module) #######################

tde_add_kpart( kcm_crypto AUTOMOC
  SOURCES crypto.cpp certexport.cpp kdatetimedlg.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
