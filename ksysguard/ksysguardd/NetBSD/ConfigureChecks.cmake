#################################################
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

# apmio
check_include_files( "sys/types.h;dev/apm/apmio.h" HAVE_DEV_APM_APMIO_H )
if( HAVE_DEV_APM_APMIO_H )
  set( HAVE_APMIO 1 CACHE INTERNAL "" FORCE )
endif( )
