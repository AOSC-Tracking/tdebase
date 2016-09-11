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
  ${CMAKE_SOURCE_DIR}/khotkeys/shared
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### kded_khotkeys (module) ####################

tde_add_kpart( kded_khotkeys AUTOMOC
  SOURCES kded.cpp kded.skel
  LINK khotkeys_shared-shared tdeinit_kded-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### khotkeys (tdeinit) ########################

tde_add_tdeinit_executable( khotkeys AUTOMOC
  SOURCES app.cpp app.skel
  LINK khotkeys_shared-shared
)
