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
  DESTINATION ${DATA_INSTALL_DIR}/tdm/themes/minimalist
  PO_DIR tdm-themes
)

install(
  FILES
    background.png
    box.png
    caps-lock-warning.png
    lineedit.png
    login_normal.png
    login_prelight.png
    minimalist.xml
    screenshot.png
    session_normal.png
    session_prelight.png
    system_normal.png
    system_prelight.png
    tdelogo.png
    topline.png

  DESTINATION
    ${DATA_INSTALL_DIR}/tdm/themes/minimalist
)
