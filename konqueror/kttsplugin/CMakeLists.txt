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

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TDE_LIBRARY_DIRS}
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE tdehtmlkttsd.desktop
  DESTINATION ${DATA_INSTALL_DIR}/tdehtml/kpartplugins
  PO_DIR konqueror-desktops
)

install(
  FILES tdehtmlkttsd.rc
  DESTINATION ${DATA_INSTALL_DIR}/tdehtml/kpartplugins
)


##### libtdehtmlkttsdplugin (module) ##############

set( target libtdehtmlkttsdplugin )

tde_add_kpart( ${target} AUTOMOC
  SOURCES tdehtmlkttsd.cpp
  LINK tdehtml-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
