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

include( ConfigureChecks.cmake )

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

install( FILES fish.protocol nxfish.protocol DESTINATION ${SERVICES_INSTALL_DIR} )


##### tdeio_fish (module) #########################

add_custom_command( OUTPUT fishcode.h
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/genfishcode.pl ${CMAKE_CURRENT_SOURCE_DIR}/fish.pl > fishcode.h
  DEPENDS fish.pl )

set_property( SOURCE fish.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fishcode.h )

set( target tdeio_fish )

tde_add_kpart( ${target} AUTOMOC
  SOURCES fish.cpp
  LINK tdeio-shared ${LIB_UTIL}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
