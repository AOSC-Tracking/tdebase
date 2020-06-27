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
  ${CMAKE_BINARY_DIR}/kcontrol/background
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${LIBART_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE tdm.desktop
  PO_DIR kcontrol-desktops
)


##### kcm_tdm (module) ##########################

set_property( SOURCE background.cpp main.cpp APPEND PROPERTY COMPILE_DEFINITIONS KDE_CONFDIR="${TDE_CONFIG_DIR}" )

tde_add_kpart( kcm_tdm AUTOMOC
  SOURCES
    background.cpp tdm-appear.cpp tdm-font.cpp tdm-shut.cpp
    tdm-users.cpp tdm-conv.cpp main.cpp kbackedcombobox.cpp
  LINK kcmbgnd-static bgnd-static tdenewstuff-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
