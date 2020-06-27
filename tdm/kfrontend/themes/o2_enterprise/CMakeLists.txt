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
  SOURCE GdmGreeterTheme.desktop
  DESTINATION ${DATA_INSTALL_DIR}/tdm/themes/o2_enterprise
  PO_DIR tdm-themes
)

install(
  FILES
    Dialog.png enter_normal.png enter_over.png enter_pressed.png
    enterprise.xml preview.png
    system_normal.png  system_over.png system_pressed.png
  DESTINATION ${DATA_INSTALL_DIR}/tdm/themes/o2_enterprise
)
