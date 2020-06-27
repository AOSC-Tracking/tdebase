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

add_subdirectory( pics )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE displayconfig.desktop
  PO_DIR kcontrol-desktops
)


##### kcm_displayconfig (module) ####################

set_property( SOURCE displayconfig.cpp APPEND PROPERTY COMPILE_DEFINITIONS KDE_CONFDIR="${TDE_CONFIG_DIR}" )

tde_add_kpart( kcm_displayconfig AUTOMOC
  SOURCES
    monitorworkspace.cpp displayconfig.cpp displayconfigbase.ui displayconfig.skel
  LINK tdeio-shared tderandr-shared tdeutils-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
