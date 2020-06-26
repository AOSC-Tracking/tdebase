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
  SOURCE kcmperformance.desktop
  PO_DIR kcontrol-desktops
)
tde_create_translated_desktop(
  SOURCE kcmkonqyperformance.desktop
  DESTINATION ${APPS_INSTALL_DIR}/.hidden
  PO_DIR kcontrol-desktops
)


##### kcm_performance (module) ##################

tde_add_kpart( kcm_performance AUTOMOC
  SOURCES kcmperformance.cpp konqueror.cpp konqueror_ui.ui system.cpp system_ui.ui
  LINK tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
