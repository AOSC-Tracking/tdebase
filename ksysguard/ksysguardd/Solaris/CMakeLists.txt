#################################################
#
#  (C) 2023 Denis Kozadaev
#  denis (AT) tambov (DOT) ru
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
    Memory.c LoadAvg.c ProcessList.c NetDev.c
)
