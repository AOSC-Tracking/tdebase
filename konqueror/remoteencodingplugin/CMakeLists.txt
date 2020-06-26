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
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TDE_LIBRARY_DIRS}
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE kremoteencodingplugin.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqiconview/kpartplugins
  PO_DIR konqueror-desktops
)

install(
  FILES kremoteencodingplugin.rc
  DESTINATION ${DATA_INSTALL_DIR}/konqiconview/kpartplugins
)

install(
  FILES kremoteencodingplugin.rc ${CMAKE_CURRENT_BINARY_DIR}/kremoteencodingplugin.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqlistview/kpartplugins
)


##### konq_remoteencoding (module) ##############

set( target konq_remoteencoding )

set( ${target}_SRCS
  kremoteencodingplugin.cpp
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK konq-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
