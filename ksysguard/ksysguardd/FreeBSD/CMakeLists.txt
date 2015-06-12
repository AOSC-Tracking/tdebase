#################################################
#
#  (C) 2015 Serghei Amelian
#  slavek (DOT) banko (AT) axis.cz
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################


include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/ksysguard/CContLib
  ${CMAKE_SOURCE_DIR}/ksysguard/ksysguardd
)


##### ksysguardd (static) #######################

tde_add_library( ksysguardd STATIC
  SOURCES
    CPU.c Memory.c ProcessList.c apm.c diskstat.c loadavg.c logfile.c netdev.c
  LINK kvm
)
