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

if( WITH_XCURSOR )
  add_subdirectory( xcursor )
  include_directories( xcursor )
else( )
  add_subdirectory( core )
  include_directories( core )
endif( )

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
  SOURCE mouse.desktop touchpad.desktop
  PO_DIR kcontrol-desktops
)

install( FILES
    cursor_large_black.pcf.gz cursor_large_white.pcf.gz
    cursor_small_white.pcf.gz
  DESTINATION ${DATA_INSTALL_DIR}/kcminput )

install( FILES
    mouse_cursor_theme.upd
  DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )

install(
    DIRECTORY pics/
    DESTINATION ${DATA_INSTALL_DIR}/kcminput/pics
    FILES_MATCHING PATTERN *.png
)

add_subdirectory(icons)


##### kcm_input (module) ########################

tde_add_kpart( kcm_input AUTOMOC
  SOURCES
    mouse.cpp kmousedlg.ui main.cpp
    logitechmouse.cpp logitechmouse_base.ui
    touchpad.cpp touchpad_settings.cpp
  LINK
    themepage-static tdeio-shared
    ${XCURSOR_LIBRARIES} ${LIBUSB_LIBRARIES}
    ${XINPUT_LIBRARIES}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### tdesyndaemon (executable) ####################

tde_add_executable( tdesyndaemon AUTOMOC
  SOURCES
    tdesyndaemon.cpp tdesyndaemon_iface.skel
    touchpad_settings.cpp
  LINK tdecore-shared ${XINPUT_LIBRARIES}
  DESTINATION ${BIN_INSTALL_DIR}
)


##### kapplymousetheme (executable) #############

tde_add_executable( kapplymousetheme
  SOURCES
    kapplymousetheme.cpp
  LINK ${TQT_LIBRARIES} ${XCURSOR_LIBRARIES} X11
  DESTINATION ${BIN_INSTALL_DIR}
)
