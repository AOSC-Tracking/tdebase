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

if( WITH_PAM )
  set( TDESCREENSAVER_PAM_SERVICE "kde" CACHE INTERNAL "" )
endif( )

# check for dbus
tde_setup_dbus( dbus-1-tqt )
