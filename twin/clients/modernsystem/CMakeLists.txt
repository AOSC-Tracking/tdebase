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
  ${CMAKE_SOURCE_DIR}/twin/lib
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE modernsystem.desktop
  DESTINATION ${DATA_INSTALL_DIR}/twin
  PO_DIR twin-clients-desktops
)


##### twin_modernsys (module) ##################

tde_add_kpart( twin_modernsys
  SOURCES modernsys.cpp
  LINK tdecorations-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
