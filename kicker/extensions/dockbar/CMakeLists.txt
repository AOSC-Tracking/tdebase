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
  SOURCE dockbarextension.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/extensions
  PO_DIR kicker-desktops
)


##### dockbar_panelextension (module) ###########

set( target dockbar_panelextension )

set( ${target}_SRCS
  dockcontainer.cpp dockbarextension.cpp
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
