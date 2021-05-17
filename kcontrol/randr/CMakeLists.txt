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
  SOURCE tderandrtray.desktop
  PO_DIR kcontrol-desktops
)

tde_create_translated_desktop(
  SOURCE randr.desktop
  DESTINATION ${APPS_INSTALL_DIR}/.hidden
  PO_DIR kcontrol-desktops
)

tde_create_translated_desktop(
  SOURCE tderandrtray-autostart.desktop
  DESTINATION ${AUTOSTART_INSTALL_DIR}
  PO_DIR kcontrol-desktops
)


##### kcm_randr (module) ########################

tde_add_kpart( kcm_randr AUTOMOC
  SOURCES tderandrmodule.cpp
  LINK tdeui-shared tderandr-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### tderandrtray (executable) ###################

tde_add_executable( tderandrtray AUTOMOC
  SOURCES
    main.cpp tderandrtray.cpp tderandrapp.cpp
    tderandrpassivepopup.cpp configdialog.cpp
  LINK tdeutils-shared tderandr-shared ${TDEHW_LIBRARIES}
  DESTINATION ${BIN_INSTALL_DIR}
)
