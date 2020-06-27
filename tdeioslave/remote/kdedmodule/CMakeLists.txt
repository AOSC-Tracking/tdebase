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
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE remotedirnotify.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}/kded
  PO_DIR tdeioslave-desktops
)


##### kded_remotedirnotify (module) #############

set( target kded_remotedirnotify )

set( ${target}_SRCS
  remotedirnotify.cpp remotedirnotify.skel
  remotedirnotifymodule.cpp remotedirnotifymodule.skel
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK tdeinit_kded-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
