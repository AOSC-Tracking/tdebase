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

# FIXME smartcard is not ported

if( BUILD_KCONTROL OR BUILD_KDESKTOP OR BUILD_TDM )

  add_subdirectory( background )

endif( )


if( BUILD_KCONTROL )

  include( ConfigureChecks.cmake )

  add_subdirectory( kcontrol )
  add_subdirectory( pics )
  add_subdirectory( bell )
  add_subdirectory( dnssd )
  add_subdirectory( filetypes )
  add_subdirectory( krdb )
  add_subdirectory( input )
  add_subdirectory( kicker )
  add_subdirectory( info )
  add_subdirectory( locale )
  add_subdirectory( clock )
  add_subdirectory( kded )
  add_subdirectory( launch )
  add_subdirectory( keys )
  add_subdirectory( tdm )
  add_subdirectory( tdeio )
  add_subdirectory( konq )
  add_subdirectory( style )
  add_subdirectory( konqhtml )
  add_subdirectory( ebrowsing )
  add_subdirectory( arts )
  add_subdirectory( knotify )
  add_subdirectory( colors )
  add_subdirectory( fonts )
  add_subdirectory( energy )
  add_subdirectory( css )
  add_subdirectory( icons )
  add_subdirectory( screensaver )
  add_subdirectory( taskbar )
  add_subdirectory( crypto )
  add_subdirectory( privacy )
  add_subdirectory( smserver )
  add_subdirectory( konsole )
  add_subdirectory( spellchecking )
  add_subdirectory( componentchooser )
  add_subdirectory( nics )
  add_subdirectory( performance )
  add_subdirectory( xinerama )
  add_subdirectory( display )
  add_subdirectory( kthememanager )
  add_subdirectory( tdefontinst )
  add_subdirectory( access )

  tde_conditional_add_subdirectory( WITH_XRANDR displayconfig )
  tde_conditional_add_subdirectory( WITH_TDEHWLIB hwmanager )
  tde_conditional_add_subdirectory( WITH_SAMBA samba )
  tde_conditional_add_subdirectory( WITH_XRANDR randr )
  tde_conditional_add_subdirectory( WITH_LIBRAW1394 view1394 )

  if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    tde_conditional_add_subdirectory( WITH_XRANDR iccconfig )
    add_subdirectory( joystick )
  endif( )

  if( ${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD|NetBSD" )
    add_subdirectory( usbview )
  endif( )

endif( )
