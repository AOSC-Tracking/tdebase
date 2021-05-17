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
  ${CMAKE_SOURCE_DIR}/tdmlib
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE medianotifier.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}/kded
  PO_DIR tdeioslave-desktops
)


##### kded_medianotifier (module) ###############

set( target kded_medianotifier )

set( ${target}_SRCS
  medianotifier.cpp medianotifier.skel notificationdialog.cpp
  notificationdialogview.ui
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK mediacommon-static tdeinit_kded-shared dmctl-static
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
