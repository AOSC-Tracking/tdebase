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

tde_create_translated_desktop(
  SOURCE
    kcm_kemail.desktop kcm_terminal.desktop kcm_browser.desktop kcm_filemanager.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kcm_componentchooser
  PO_DIR kcontrol-desktops
)
