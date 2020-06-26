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
  SOURCE
    home.desktop documents.desktop users.desktop
    media.desktop remote.desktop trash.desktop
  DESTINATION ${DATA_INSTALL_DIR}/systemview
  PO_DIR tdeioslave-desktops
)
