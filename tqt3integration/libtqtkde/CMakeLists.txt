#################################################
#
#  (C) 2011 Timothy Pearson
#  kb9vqf (AT) pearsoncomputing.net
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/tqt3integration/utils/
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### libtqtkde (shared) ####################

tde_add_library( tqtkde SHARED AUTOMOC
  SOURCES tqtkde.cpp
  VERSION 0.0.0
  LINK tdeui-shared
  DEPENDENCIES generate_tqt3_bindings
  DESTINATION "${PLUGIN_INSTALL_DIR}/plugins/integration/"
)
