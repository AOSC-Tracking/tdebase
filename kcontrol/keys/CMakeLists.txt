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
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE keys.desktop
  PO_DIR kcontrol-desktops
)
install( FILES tdeaccel.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )
install( PROGRAMS convertShortcuts.pl DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )

install( FILES
    wm3.kksrc trinity.kksrc kde4.kksrc win3.kksrc
    win4.kksrc mac4.kksrc unix3.kksrc
  DESTINATION ${DATA_INSTALL_DIR}/kcmkeys )


##### kcm_keys (module) #########################

tde_add_kpart( kcm_keys AUTOMOC
  SOURCES
    shortcuts.cpp modifiers.cpp main.cpp khotkeys.cpp
    treeview.cpp commandShortcuts.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
