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

add_subdirectory( Templates )

tde_create_translated_desktop(
  SOURCE
    directory.autostart directory.templates directory.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kdesktop
  PO_DIR kdesktop-icons
)

tde_create_translated_desktop(
  SOURCE
    My_Computer My_Documents My_Network_Places Printers Trash Web_Browser
  DESTINATION ${DATA_INSTALL_DIR}/kdesktop/Desktop
  PO_DIR kdesktop-icons
)

tde_create_translated_desktop(
  SOURCE
    Home.desktop System.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kdesktop/DesktopLinks
  PO_DIR kdesktop-icons
)
