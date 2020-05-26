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
    check_include_file( openssl/ssl.h HAVE_OPENSSL_H )
    check_library_exists( ssl OPENSSL_init_ssl "" HAVE_LIBSSL_11 )
    if( NOT HAVE_LIBSSL_11 )
      check_library_exists( ssl SSL_library_init "" HAVE_LIBSSL )
    endif( )
    check_library_exists( crypto EVP_EncryptInit_ex "" HAVE_LIBCRYPTO )
    if( HAVE_OPENSSL_H AND (HAVE_LIBSSL_11 OR HAVE_LIBSSL) AND HAVE_LIBCRYPTO )
      set( SSL_FOUND 1 CACHE INTERNAL "" FORCE )
      find_file( OPENSSLV_H openssl/opensslv.h )
      file( STRINGS "${OPENSSLV_H}" SSL_VERSION REGEX "#[ \t]*define[ \t]*OPENSSL_VERSION_TEXT" )
      string( REGEX REPLACE "# *define[ \t]*OPENSSL_VERSION_TEXT[ \t]*\"[^0-9 ]* *([^ ]*).*" "\\1"
              SSL_VERSION "${SSL_VERSION}" )
      set( SSL_VERSION "${SSL_VERSION}" CACHE INTERNAL "" FORCE )
      message( STATUS "Found OpenSSL: version ${SSL_VERSION}" )
    endif( HAVE_OPENSSL_H AND (HAVE_LIBSSL_11 OR HAVE_LIBSSL) AND HAVE_LIBCRYPTO )
  endif( NOT SSL_FOUND )
  if( NOT SSL_FOUND )
    tde_message_fatal( "SSL support is requested, but openssl not found on your system" )
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
