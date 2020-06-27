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

add_subdirectory( locolor )

tde_create_translated_desktop(
  SOURCE Theme.rc
  DESTINATION ${DATA_INSTALL_DIR}/ksplash/Themes/TDE-Classic
  PO_DIR ksplash-themes
)

install(
  FILES
    Preview.png splash_active_bar.png splash_bottom.png
    splash_inactive_bar.png splash_top.png
  DESTINATION ${DATA_INSTALL_DIR}/ksplash/Themes/TDE-Classic
)
