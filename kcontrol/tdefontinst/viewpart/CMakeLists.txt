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
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/kcontrol/tdefontinst/lib
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE tdefontviewpart.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR kcontrol-desktops
)

tde_create_translated_desktop(
  SOURCE tdefontview.desktop
  PO_DIR kcontrol-desktops
)

install( FILES tdefontviewpart.rc tdefontviewui.rc DESTINATION ${DATA_INSTALL_DIR}/tdefontview )


##### libtdefontviewpart (module) #################

tde_add_kpart( libtdefontviewpart AUTOMOC
  SOURCES FontViewPart.cpp FontViewPartFactory.cpp FontPreview.cpp
  LINK tdefontinstprint-static tdefontinst-shared tdeparts-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### tdefontview (executable) ####################

tde_add_executable( tdefontview AUTOMOC
  SOURCES FontViewerApp.cpp
  LINK tdeparts-shared
  DESTINATION ${BIN_INSTALL_DIR}
)


##### tdefontinstprint (static) ###################

tde_add_library( tdefontinstprint STATIC_PIC
  SOURCES KfiPrint.cpp
  LINK tdeprint-shared
)
