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
  ${CMAKE_BINARY_DIR}/tdeioslave/media/libmediacommon
  ${CMAKE_SOURCE_DIR}/tdeioslave/media/libmediacommon
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE mediamanager.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}/kded
  PO_DIR tdeioslave-desktops
)

tde_create_translated_desktop(
  SOURCE mediabackend.desktop
  DESTINATION ${AUTOSTART_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)


##### kded_mediamanager (module) ################

set( target kded_mediamanager )

if( WITH_TDEHWLIB )
  set( TDEHWBACKEND tdehardwarebackend.cpp )
endif( WITH_TDEHWLIB )

tde_add_kpart( ${target} AUTOMOC
  SOURCES
    mediamanager.cpp mediamanager.skel medialist.cpp
    backendbase.cpp fstabbackend.cpp removablebackend.cpp
    unlockdialog.ui dialog.cpp
    mediadirnotify.cpp mediadirnotify.skel
    ${TDEHWBACKEND}
  LINK mediacommon-static tdeinit_kded-shared ${TDEHW_LIBRARIES}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
