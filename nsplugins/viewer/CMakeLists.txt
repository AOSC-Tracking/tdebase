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
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/nsplugins
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${GLIB2_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${GLIB2_LIBRARY_DIRS}
)


##### nspluginviewer (executable) ###############

tde_add_executable( nspluginviewer AUTOMOC
  SOURCES
    ../NSPluginCallbackIface.stub NSPluginClassIface.skel
    nsplugin.cpp viewer.cpp qxteventloop.cpp
    glibevents.cpp
  LINK tdeparts-shared tdeio-shared ${GLIB2_LIBRARIES} Xt ${DL_LIBRARIES}
  DESTINATION ${BIN_INSTALL_DIR}
)
