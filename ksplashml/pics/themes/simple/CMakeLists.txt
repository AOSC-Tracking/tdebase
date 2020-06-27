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
  SOURCE Theme.rc
  DESTINATION ${DATA_INSTALL_DIR}/ksplash/Themes/Simple
  PO_DIR ksplash-themes
)

install(
  FILES Preview.png
  DESTINATION ${DATA_INSTALL_DIR}/ksplash/Themes/Simple
)
