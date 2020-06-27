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
  SOURCE fontthumbnail.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR kcontrol-desktops
)


##### fontthumbnail (module) ####################

tde_add_kpart( fontthumbnail
  SOURCES FontThumbnail.cpp
  LINK tdefontinst-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
