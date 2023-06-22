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

add_subdirectory( config )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/twin/lib
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE keramik.desktop
  DESTINATION ${DATA_INSTALL_DIR}/twin
  PO_DIR twin-clients-desktops
)


##### twin_keramik (module) ####################

set_source_files_properties( keramik.cpp
  PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tiles.h )

tde_add_kpart( twin_keramik AUTOMOC
  SOURCES keramik.cpp
  LINK tdecorations-shared tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### tiles.h (header) ##########################

file(GLOB _pics "${CMAKE_CURRENT_SOURCE_DIR}/pics/*.png" )

add_custom_command( OUTPUT tiles.h
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/embedtool ${_pics}
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/embedtool ${_pics} )

tde_add_executable( embedtool
  SOURCES embedtool.cpp
  LINK ${TQT_LIBRARIES}
)
