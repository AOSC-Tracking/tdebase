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

add_subdirectory( ksgrd )
add_subdirectory( SensorDisplayLib )


include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/SensorDisplayLib
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/ksgrd
  ${CMAKE_CURRENT_SOURCE_DIR}/SensorDisplayLib
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE ksysguard.desktop
  DESTINATION ${XDG_APPS_INSTALL_DIR}
  PO_DIR ksysguard-desktops
)
tde_create_translated_desktop(
  SOURCE x-ksysguard.desktop
  DESTINATION ${MIME_INSTALL_DIR}/application
  PO_DIR ksysguard-desktops
)
install( FILES ProcessTable.sgrd SystemLoad.sgrd KSysGuardApplet.xml ksysguardui.rc DESTINATION ${DATA_INSTALL_DIR}/ksysguard )
tde_create_translated_desktop(
  SOURCE ksysguardapplet.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/applets
  PO_DIR ksysguard-desktops
)


##### sysguard_panelapplet (module) #############

tde_add_kpart( sysguard_panelapplet AUTOMOC
  SOURCES KSysGuardApplet.cpp KSGAppletSettings.cpp
  LINK sensordisplays-static ksgrd-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### ksysguard (executable) ####################

tde_add_executable( ksysguard AUTOMOC
  SOURCES
    SensorBrowser.cpp WorkSheet.cpp WorkSheetSettings.cpp
    Workspace.cpp ksysguard.cpp ksysguard.skel
  LINK sensordisplays-static ksgrd-shared tdednssd-shared
  DESTINATION ${BIN_INSTALL_DIR}
)


##### kpm (executable) ##########################

tde_add_executable( kpm
  SOURCES kpm.c
  DESTINATION ${BIN_INSTALL_DIR}
)
