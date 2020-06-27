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
    .directory audiocd.desktop printsystem.desktop
    media.desktop settings.desktop applications.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/services
  PO_DIR konqueror-desktops
)
