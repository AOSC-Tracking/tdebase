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

if( BUILD_KCONTROL OR BUILD_KDESKTOP OR BUILD_KDM )

  include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/libkonq
    ${TDE_INCLUDE_DIR}
    ${TQT_INCLUDE_DIRS}
    ${LIBART_INCLUDE_DIRS}
  )

  link_directories(
    ${TQT_LIBRARY_DIRS}
  )

  ##### bgnd (static) #############################

  tde_add_library( bgnd STATIC_PIC AUTOMOC
    SOURCES bgrender.cpp bgsettings.cpp
    LINK ${LIBART_LIBRARIES}
  )

endif( )


if( BUILD_KCONTROL )

  add_subdirectory( pics )

  ##### other data ################################

  install( FILES background.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )


  ##### kcm_background (module) ###################

  tde_add_kpart( kcm_background AUTOMOC
    SOURCES main.cpp
    LINK kcmbgnd-static bgnd-static knewstuff-shared
    DESTINATION ${PLUGIN_INSTALL_DIR}
  )


  ##### kcmbgnd (static) ##########################

  tde_add_library( kcmbgnd STATIC_PIC AUTOMOC
    SOURCES
      bgdialog.cpp bgdialog_ui.ui bgwallpaper_ui.ui
      bgwallpaper.cpp bgadvanced_ui.ui bgadvanced.cpp
      bgmonitor.cpp
  )

endif( )
