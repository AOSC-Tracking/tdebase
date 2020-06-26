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
  SOURCE media.desktop
  PO_DIR tdeioslave-desktops
)


##### kcm_media (module) ########################

set( target kcm_media )

set( ${target}_SRCS
  notifiermodule.cpp notifiermoduleview.ui serviceconfigdialog.cpp
  serviceview.ui mimetypelistboxitem.cpp managermodule.cpp
  managermoduleview.ui main.cpp
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK mediacommon-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
