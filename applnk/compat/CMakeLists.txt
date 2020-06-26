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
    battery.desktop bwarning.desktop cwarning.desktop
    power.desktop kcmkonq.desktop kcmkxmlrpcd.desktop
    konqhtml.desktop socks.desktop virtualdesktops.desktop
    email.desktop userinfo.desktop passwords.desktop
  DESTINATION ${APPS_INSTALL_DIR}/.hidden
  PO_DIR applnk-compat
)
