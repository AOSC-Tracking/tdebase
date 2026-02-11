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
  ${CMAKE_BINARY_DIR}/kicker/libkicker
  ${CMAKE_SOURCE_DIR}/kicker/libkicker
  ${CMAKE_SOURCE_DIR}/kicker/kicker/ui
  ${CMAKE_SOURCE_DIR}/kicker/kicker/core
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${DBUS_TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE quicklauncher.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/applets
  PO_DIR kicker-desktops
)
install( FILES launcherapplet.kcfg DESTINATION ${KCFG_INSTALL_DIR} )


##### launcher_panelapplet (module) #############

# NOTE: There we have a circular dependency,
#       for this reason I used -Wl,--start-group / -Wl,--end-group

tde_add_kpart( launcher_panelapplet AUTOMOC
  SOURCES
    quicklauncher.skel quicklauncher.cpp quickbutton.cpp
    quickaddappsmenu.cpp flowgridmanager.cpp popularity.cpp
    configdlgbase.ui prefs.kcfgc configdlg.cpp
  LINK
    -Wl,--start-group
      kicker_core-static kicker_buttons-static kicker_ui-static
    -Wl,--end-group
    kickermain-shared tdeutils-shared tdeabc-shared
  DEPENDENCIES kickermain-shared kicker+libkicker+kickerSettings.cpp
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
