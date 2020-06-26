#################################################
#
#  (C) 2012 - 2015 Timothy Pearson
#  kb9vqf (AT) pearsoncomputing.net
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
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
  SOURCE hwmanager.desktop
  PO_DIR kcontrol-desktops
)
tde_create_translated_desktop(
  SOURCE hwdevicetray.desktop
  OUTPUT_NAME tdehwdevicetray.desktop
  PO_DIR kcontrol-desktops
)
tde_create_translated_desktop(
  SOURCE hwdevicetray-autostart.desktop
  DESTINATION ${AUTOSTART_INSTALL_DIR}
  OUTPUT_NAME tdehwdevicetray-autostart.desktop
  PO_DIR kcontrol-desktops
)


##### kcm_iccconfig (module) ####################

set_property( SOURCE hwmanager.cpp APPEND PROPERTY COMPILE_DEFINITIONS KDE_CONFDIR="${TDE_CONFIG_DIR}" )

tde_add_kpart( kcm_hwmanager AUTOMOC
  SOURCES
    hwmanager.cpp deviceiconview.cpp devicepropsdlg.cpp devicepropsdlgbase.ui hwmanagerbase.ui
    cryptpassworddlg.cpp cryptpassworddlgbase.ui hwmanager.skel
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)

##### tdehwdevicetray (executable) ##############

tde_add_executable( tdehwdevicetray AUTOMOC
  SOURCES
    hwdevicetray_main.cpp hwdevicetray.cpp hwdevicetray_app.cpp
    hwdevicetray_configdialog.cpp
  LINK tdeio-shared tdeutils-shared tdeui-shared
  DESTINATION ${BIN_INSTALL_DIR}
)