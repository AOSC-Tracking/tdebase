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
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE twinoptions.desktop
  PO_DIR twin-desktops
)

tde_create_translated_desktop(
  SOURCE
    twinactions.desktop twinadvanced.desktop twinfocus.desktop
    twinmoving.desktop twintranslucency.desktop
  DESTINATION ${APPS_INSTALL_DIR}/.hidden
  PO_DIR twin-desktops
)


##### kcm_twinoptions (module) ##################

tde_add_kpart( kcm_twinoptions AUTOMOC
  SOURCES windows.cpp mouse.cpp main.cpp
  LINK tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
