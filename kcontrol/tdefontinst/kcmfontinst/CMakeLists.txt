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
  ${CMAKE_SOURCE_DIR}/kcontrol/tdefontinst/viewpart
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_install_icons()
tde_create_translated_desktop(
  SOURCE kcmfontinst.desktop
  PO_DIR kcontrol-desktops
)


##### kcm_fontinst (module) #####################

tde_add_kpart( kcm_fontinst AUTOMOC
  SOURCES
    KCmFontInst.cpp KFileFontIconView.cpp KFileFontView.cpp
    PrintDialog.cpp SettingsDialog.cpp
  LINK tdefontinstprint-static tdefontinst-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
