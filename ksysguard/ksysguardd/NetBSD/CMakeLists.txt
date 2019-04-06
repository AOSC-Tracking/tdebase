#################################################
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

include( ConfigureChecks.cmake )

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/ksysguard/CContLib
  ${CMAKE_SOURCE_DIR}/ksysguard/ksysguardd
)


##### ksysguardd (static) #######################

if ( HAVE_APMIO )
  set( APM_SOURCES apm.c )
endif ( HAVE_APMIO )

tde_add_library( ksysguardd STATIC
  SOURCES
    CPU.c Memory.c ProcessList.c ${APM_SOURCES} diskstat.c loadavg.c logfile.c netdev.c
  LINK kvm
)
