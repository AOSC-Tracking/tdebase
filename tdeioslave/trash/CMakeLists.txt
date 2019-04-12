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

add_subdirectory( tdefile-plugin )


include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES ktrashpropsdlgplugin.desktop trash.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
install( FILES kcm_trash.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )


##### trashcommon (static) ######################
set( target trashcommon )

tde_add_library( ${target} STATIC_PIC AUTOMOC
  SOURCES trashimpl.cpp discspaceutil.cpp ktrashpropswidget.cpp
)


##### tdeio_trash (module) ########################
set( target tdeio_trash )

tde_add_kpart( ${target} AUTOMOC
  SOURCES tdeio_trash.cpp
  LINK trashcommon-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### kcm_trash (module) ########################
set( target kcm_trash )

tde_add_kpart( ${target} AUTOMOC
  SOURCES kcm_trash.cpp
  LINK trashcommon-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### ktrashpropsdlgplugin (module) ########################
set( target ktrashpropsdlgplugin )

tde_add_kpart( ${target} AUTOMOC
  SOURCES ktrashpropsdlgplugin.cpp
  LINK trashcommon-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### ktrash (executable) #######################
set( target ktrash )

tde_add_executable( ${target}
  SOURCES ktrash.cpp
  LINK tdeio-shared
  DESTINATION ${BIN_INSTALL_DIR}
)


##### testtrash (executable) #######################
tde_add_executable( testtrash
  SOURCES testtrash.cpp AUTOMOC
  LINK trashcommon-static tdeio-shared
)
