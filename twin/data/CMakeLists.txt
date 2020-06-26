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
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES
    twin.upd twinsticky.upd twiniconify.upd twin3_plugin.upd
    twin_focus1.upd twinupdatewindowsettings.upd
    twin_focus2.upd twin_fsp_workarounds_1.upd
  DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )

install( PROGRAMS
    pluginlibFix.pl twin3_plugin.pl twin_focus1.sh twin_focus2.sh
  DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )

install( FILES
    pop.wav
  DESTINATION ${SOUND_INSTALL_DIR} )

tde_create_translated_desktop(
  SOURCE fsp_workarounds_1.twinrules
  DESTINATION ${DATA_INSTALL_DIR}/twin/default_rules
  PO_DIR twin-desktops
)


##### twin_update_window_settings (executable) ##

tde_add_executable( twin_update_window_settings
  SOURCES update_window_settings.cpp
  LINK tdecore-shared
  DESTINATION ${LIB_INSTALL_DIR}/tdeconf_update_bin
)


##### twin_update_default_rules (executable) ####

tde_add_executable( twin_update_default_rules
  SOURCES update_default_rules.cpp
  LINK tdecore-shared
  DESTINATION ${LIB_INSTALL_DIR}/tdeconf_update_bin
)
