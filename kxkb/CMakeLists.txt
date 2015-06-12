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

include( ConfigureChecks.cmake )

add_subdirectory( pics )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${XTEST_LIBDIR}
)


##### other data ################################

install( FILES keyboard.desktop keyboard_layout.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
install( FILES kxkb_groups DESTINATION ${CONFIG_INSTALL_DIR} )
install( FILES kxkb.desktop DESTINATION ${SERVICES_INSTALL_DIR} )


##### kcm_keyboard (module) #####################

tde_add_kpart( kcm_keyboard AUTOMOC
  SOURCES
    extension.cpp x11helper.cpp rules.cpp kxkbconfig.cpp
    pixmap.cpp kcmlayout.cpp kcmlayoutwidget.ui
    kcmmisc.cpp kcmmiscwidget.ui
  LINK tdeio-shared ${XKBFILE_LIBRARIES} ${XTEST_LIBRARIES}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### kxkb (tdeinit) ############################

tde_add_tdeinit_executable( kxkb AUTOMOC
  SOURCES
    extension.cpp x11helper.cpp rules.cpp kxkbconfig.cpp
    pixmap.cpp layoutmap.cpp kxkb.cpp kxkbtraywindow.cpp
    kxkb.skel
  LINK tdeui-shared ${XKBFILE_LIBRARIES}
)
