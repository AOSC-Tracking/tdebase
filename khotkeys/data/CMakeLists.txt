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
    trinity2b1.khotkeys konqueror_gestures_trinity21.khotkeys
    printscreen.khotkeys
  DESTINATION ${DATA_INSTALL_DIR}/khotkeys
  PO_DIR khotkeys-data
)

install( FILES
    khotkeys_32b1_update.upd khotkeys_printscreen.upd
    konqueror_gestures_trinity21_update.upd
  DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )
