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
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/kicker/libkicker
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/kicker/libkicker
  ${CMAKE_SOURCE_DIR}/kicker/kicker/ui
  ${CMAKE_SOURCE_DIR}/kicker/kicker/buttons
  ${DBUS_TQT_INCLUDE_DIRS}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES default-apps DESTINATION ${DATA_INSTALL_DIR}/kicker )
tde_create_translated_desktop(
  SOURCE childpanelextension.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/extensions
  PO_DIR kicker-desktops
)


##### kicker_core (static) ######################

set( target kicker_core )

set( ${target}_SRCS
  extensionSettings.kcfgc main.cpp kicker.cpp kicker.skel
  userrectsel.cpp usersizesel.cpp containerarea.cpp kmenubase.ui
  applethandle.cpp container_base.cpp container_button.cpp
  container_applet.cpp container_extension.cpp extensionmanager.cpp
  menumanager.cpp pluginmanager.cpp showdesktop.cpp
  unhidetrigger.cpp containerarealayout.cpp
  panelextension.cpp panelextension.skel
)

tde_add_library( ${target} STATIC_PIC AUTOMOC
  SOURCES ${${target}_SRCS}
  DEPENDENCIES kickermain-shared kicker+libkicker+kickerSettings.cpp
  LINK konq-shared
)
