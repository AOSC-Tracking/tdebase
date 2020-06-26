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
    folder.desktop printer.desktop class.desktop
    manager.desktop jobs.desktop driver.desktop
    printermodel.desktop
  DESTINATION ${MIME_INSTALL_DIR}/print
  PO_DIR tdeprint-desktops
)
