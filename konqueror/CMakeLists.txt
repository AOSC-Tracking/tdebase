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

project( konqueror )

if( NOT BUILD_LIBKONQ )
  include( "${TDE_CMAKE_DIR}/libkonq.cmake" )
endif( NOT BUILD_LIBKONQ )


add_subdirectory( client )
add_subdirectory( iconview )
add_subdirectory( listview )
add_subdirectory( keditbookmarks )
add_subdirectory( shellcmdplugin )
add_subdirectory( about )
add_subdirectory( pics )
add_subdirectory( sidebar )
add_subdirectory( preloader )
add_subdirectory( quickprint )
add_subdirectory( remoteencodingplugin )
add_subdirectory( kttsplugin )


include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TDE_LIBRARY_DIRS}
  ${TQT_LIBRARY_DIRS}
)


##### headers ###################################

install( FILES
    KonquerorIface.h
  DESTINATION ${INCLUDE_INSTALL_DIR} )


##### other data ################################

install( FILES konqueror.rc konq-simplebrowser.rc DESTINATION ${DATA_INSTALL_DIR}/konqueror )

tde_create_translated_desktop(
  SOURCE konqueror.desktop
  DESTINATION ${APPS_INSTALL_DIR}
  PO_DIR konqueror-desktops
)

tde_create_translated_desktop(
  SOURCE konqfilemgr.desktop
  DESTINATION ${APPS_INSTALL_DIR}/.hidden
  PO_DIR konqueror-desktops
)

install( FILES konqueror.kcfg DESTINATION ${KCFG_INSTALL_DIR} )

tde_create_translated_desktop(
  SOURCE
    kfmclient.desktop kfmclient_dir.desktop kfmclient_html.desktop
    kfmclient_war.desktop konqbrowser.desktop konquerorsu.desktop
    Home.desktop
  PO_DIR konqueror-desktops
)

set( PROFILEDIR ${DATA_INSTALL_DIR}/konqueror/profiles )

tde_create_translated_desktop(
  SOURCE profile_webbrowsing.desktop
  DESTINATION ${PROFILEDIR}
  OUTPUT_NAME webbrowsing
  PO_DIR konqueror-desktops
)

tde_create_translated_desktop(
  SOURCE profile_filemanagement.desktop
  DESTINATION ${PROFILEDIR}
  OUTPUT_NAME filemanagement
  PO_DIR konqueror-desktops
)

tde_create_translated_desktop(
  SOURCE profile_midnightcommander.desktop
  DESTINATION ${PROFILEDIR}
  OUTPUT_NAME midnightcommander
  PO_DIR konqueror-desktops
)

tde_create_translated_desktop(
  SOURCE profile_tabbedbrowsing.desktop
  DESTINATION ${PROFILEDIR}
  OUTPUT_NAME tabbedbrowsing
  PO_DIR konqueror-desktops
)

tde_create_translated_desktop(
  SOURCE profile_kde_devel.desktop
  DESTINATION ${PROFILEDIR}
  OUTPUT_NAME kde_devel
  PO_DIR konqueror-desktops
)

tde_create_translated_desktop(
  SOURCE profile_simplebrowser.desktop
  DESTINATION ${PROFILEDIR}
  OUTPUT_NAME simplebrowser
  PO_DIR konqueror-desktops
)


##### konqueror_intern (static lib) #############

set( target konqueror_intern )

set( ${target}_SRCS
  konq_settingsxt.kcfgc
)

tde_add_library( ${target} STATIC_PIC
  SOURCES ${${target}_SRCS}
)


##### konqueror (tdeinit) #######################

set( target konqueror )

set( ${target}_SRCS
  konq_main.cpp KonquerorIface.cpp KonquerorIface.skel
  KonqMainWindowIface.cpp KonqMainWindowIface.skel
  KonqViewIface.cpp KonqViewIface.skel konq_guiclients.cpp
  konq_run.cpp konq_view.cpp konq_viewmgr.cpp konq_misc.cpp
  konq_frame.cpp konq_tabs.cpp konq_actions.cpp konq_profiledlg.cpp
  konq_factory.cpp konq_combo.cpp	konq_browseriface.cpp
  delayedinitializer.cpp konq_mainwindow.cpp konq_extensionmanager.cpp
)

tde_add_tdeinit_executable( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK konqueror_intern-static konq-shared tdeutils-shared
)


##### install import cmake modules ###############

tde_install_export( )
