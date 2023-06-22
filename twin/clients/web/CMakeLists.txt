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
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE web.desktop
  DESTINATION ${DATA_INSTALL_DIR}/twin
  PO_DIR twin-clients-desktops
)


##### twin_web (module) ########################

tde_add_kpart( twin_web AUTOMOC
  SOURCES Web.cpp WebButton.cpp
  LINK tdecorations-shared tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
