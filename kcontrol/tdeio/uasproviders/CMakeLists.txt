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
    ie401onwinnt4.desktop ie45onwinnt5.desktop ie501onwinnt5.desktop
    ie50onppc.desktop ie55onwin98.desktop ie55onwinnt5.desktop
    ie60onwinnt51.desktop ie60oncurrent.desktop
    lynxoncurrent.desktop w3moncurrent.desktop
    ns71oncurrent.desktop ns71onwinnt51.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}/useragentstrings
  PO_DIR kcontrol-desktops
)
