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

if( WITH_SASL )
  check_include_file( "sasl/sasl.h" HAVE_SASL_SASL_H )
  if( NOT HAVE_SASL_SASL_H )
    find_path( SASL_H_PATH "sasl/sasl.h" )
    if( SASL_H_PATH )
      set( HAVE_SASL_SASL_H "1" )
    endif( )
  endif( )
  check_library_exists( sasl2 sasl_client_init "" HAVE_LIBSASL2 )
  if( HAVE_SASL_SASL_H AND HAVE_LIBSASL2 )
    set( SASL_LIBRARIES sasl2 )
  else( )
    tde_message_fatal( "sasl2 are requested, but not found on your system" )
  endif( )
endif( )

# rpc/rpc.h, originally was shipped with glibc ...
check_include_file( rpc/rpc.h HAVE_RPC_H )
# ... but later might be not present (deprecated from 2.26)
if( NOT HAVE_RPC_H )
  pkg_search_module( TIRPC libtirpc )
  if( NOT TIRPC_FOUND )
    tde_message_fatal( "rpc/rpc.h is required, please check your glibc or libtirpc package" )
  endif( )
endif( )

message( STATUS "Looking for rpcgen" )
find_program( RPCGEN_BINARY rpcgen )
if( RPCGEN_BINARY )
  message( STATUS "Looking for rpcgen - ${RPCGEN_BINARY}" )
else( RPCGEN_BINARY )
  tde_message_fatal( "rpcgen is required, but not found on your system" )
endif( RPCGEN_BINARY )
