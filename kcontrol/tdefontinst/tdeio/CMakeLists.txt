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
  ${CMAKE_SOURCE_DIR}/kcontrol/fonts
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${FONTCONFIG_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES fonts.protocol DESTINATION ${SERVICES_INSTALL_DIR} )

tde_create_translated_desktop(
  SOURCE fonts.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/services
  PO_DIR kcontrol-desktops
)

tde_create_translated_desktop(
  SOURCE folder.desktop system-folder.desktop package.desktop
  DESTINATION ${MIME_INSTALL_DIR}/fonts
  PO_DIR kcontrol-desktops
)


##### tdeio_fonts (module) #######################

tde_add_kpart( tdeio_fonts AUTOMOC
  SOURCES KioFonts.cpp
  LINK kxftconfig-static tdefontinst-shared tdesu-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
