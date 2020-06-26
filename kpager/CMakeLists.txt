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

##### icons #######################

tde_install_icons()

##### other data ################################

tde_create_translated_desktop( kpager.desktop )
install( FILES uninstall.desktop DESTINATION ${APPS_INSTALL_DIR}/Utilities RENAME kpager.desktop )


##### kpager (executable) #######################

tde_add_executable( kpager AUTOMOC
  SOURCES
    desktop.cpp kpager.cpp config.cpp windowdrag.cpp
    kpagerIface.skel main.cpp
  LINK tdeui-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
