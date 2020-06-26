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


##### service desktop file(s) ##########################

tde_create_translated_desktop(
  SOURCE tdeintegration.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}/kded
)


##### kded_tdeintegration (module) #####################

set( target kded_tdeintegration )

set( ${target}_SRCS
  module.cpp
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK tdeinit_kded-shared
  DEPENDENCIES generate_tqt3_bindings
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
