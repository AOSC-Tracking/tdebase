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
    tde.desktop.cmake admin.desktop
    gnome.desktop 9wm.desktop aewm++.desktop aewm.desktop
    afterstep.desktop amaterus.desktop amiwm.desktop
    asclassic.desktop blackbox.desktop cde.desktop
    ctwm.desktop cwwm.desktop enlightenment.desktop
    evilwm.desktop fluxbox.desktop flwm.desktop fvwm.desktop
    fvwm95.desktop golem.desktop icewm.desktop ion.desktop
    larswm.desktop lwm.desktop matchbox.desktop metacity.desktop
    mwm.desktop olvwm.desktop olwm.desktop openbox.desktop
    oroborus.desktop phluid.desktop pwm.desktop qvwm.desktop
    ratpoison.desktop sapphire.desktop sawfish.desktop
    twm.desktop ude.desktop vtwm.desktop w9wm.desktop
    waimea.desktop wm2.desktop wmaker.desktop xfce.desktop
    xfce4.desktop kde-plasma-safe.desktop kde-plasma.desktop
  DESTINATION ${DATA_INSTALL_DIR}/tdm/sessions
  PO_DIR tdm-sessions
)
