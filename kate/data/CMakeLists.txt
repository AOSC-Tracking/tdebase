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

##### other data ################################

tde_create_translated_desktop(
  SOURCE kateplugin.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR kate-desktops
)

tde_create_translated_desktop(
  SOURCE kate.desktop kwrite.desktop
  PO_DIR kate-desktops
)

install( FILES kateui.rc externaltools tips DESTINATION ${DATA_INSTALL_DIR}/kate )
install( FILES kwriteui.rc DESTINATION ${DATA_INSTALL_DIR}/kwrite )
install( FILES kate-2.4.upd DESTINATION ${DATA_INSTALL_DIR}/tdeconf_update )
install( FILES katerc DESTINATION ${CONFIG_INSTALL_DIR} )
