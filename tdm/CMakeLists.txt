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

# FIXME initial work, only PAM
# FIXME needs more checks (for kerberos, etc)
# FIXME Xsession need some improvements

if( NOT DEFINED TDM_PAM_SERVICE )
  set( TDM_PAM_SERVICE "kde" CACHE INTERNAL "" )
endif( )

include( ConfigureChecks.cmake )

add_subdirectory( backend )
add_subdirectory( kfrontend )
if( WITH_TDEHWLIB )
  tde_conditional_add_subdirectory( WITH_TDM_CRYPTOCARDWATCHER cryptocardwatcher )
endif( )

# systemd
if( BUILD_TDM_SYSTEMD_UNIT_FILE AND NOT "${SYSTEMDSYSTEMUNITDIR}" STREQUAL "" )
  configure_file( "${TDM_SERVICE_FILE_TEMPLATE}" tdm.service @ONLY )
  install( FILES "${CMAKE_CURRENT_BINARY_DIR}/tdm.service" DESTINATION ${SYSTEMDSYSTEMUNITDIR} )
endif( )
