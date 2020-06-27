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

add_definitions( -DQT_PLUGIN )

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
  SOURCE plastik.desktop
  DESTINATION ${DATA_INSTALL_DIR}/twin
  PO_DIR twin-clients-desktops
)


##### twin3_plastik (module) ####################

tde_add_kpart( twin3_plastik AUTOMOC
  SOURCES plastik.cpp plastikclient.cpp plastikbutton.cpp misc.cpp
  LINK tdecorations-shared tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
