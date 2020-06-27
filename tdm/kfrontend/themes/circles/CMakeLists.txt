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
  DESTINATION ${DATA_INSTALL_DIR}/tdm/themes/circles
  PO_DIR tdm-themes
)

install(
  FILES
    circles.xml background.svg
    flower.png help.png options.png screenshot.png
  DESTINATION ${DATA_INSTALL_DIR}/tdm/themes/circles
)
