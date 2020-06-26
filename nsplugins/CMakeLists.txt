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

add_subdirectory( viewer )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop( tdehtml_plugins.desktop )
install( FILES nspluginpart.rc DESTINATION ${DATA_INSTALL_DIR}/plugin )


##### libnsplugin (module) ######################

tde_add_kpart( libnsplugin AUTOMOC
  SOURCES
    viewer/NSPluginClassIface.stub
    NSPluginCallbackIface.skel
    plugin_part.cpp nspluginloader.cpp
  LINK tdeparts-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### kcm_nsplugins (module) ####################

tde_add_kpart( kcm_nsplugins AUTOMOC
  SOURCES kcm_nsplugins.cpp
  LINK pluginpaths-static tdecore-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### nspluginscan (executable) #################

tde_add_executable( nspluginscan AUTOMOC
  SOURCES pluginscan.cpp
  LINK pluginpaths-static tdeio-shared Xt
  DESTINATION ${BIN_INSTALL_DIR}
)


##### pluginpaths (static) ######################

tde_add_library( pluginpaths STATIC_PIC
  SOURCES plugin_paths.cpp
)
