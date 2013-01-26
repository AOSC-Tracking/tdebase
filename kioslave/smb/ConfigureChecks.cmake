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

pkg_search_module ( SMBCLIENT smbclient )
if( SMBCLIENT_FOUND ) 
  set( HAVE_LIBSMBCLIENT_H 1 )
else( )
  check_include_file( libsmbclient.h HAVE_LIBSMBCLIENT_H )
endif( )

if( HAVE_LIBSMBCLIENT_H )
  set( SMBCLIENT_LIBRARIES smbclient )
  check_library_exists( ${SMBCLIENT_LIBRARIES} smbc_new_context "" HAVE_SMBCLIENT )
endif( )

if( NOT HAVE_LIBSMBCLIENT_H OR NOT HAVE_SMBCLIENT )
  tde_message_fatal( "smbclient is requested, but was not found on your system." )
endif( )
