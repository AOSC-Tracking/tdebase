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


##### getopt.h ##################################

check_include_file( getopt.h HAVE_GETOPT_H )


##### check for freetype2 #######################

pkg_search_module( FREETYPE freetype2 )
if( FREETYPE_FOUND )
  set( HAVE_FREETYPE2 1 CACHE INTERNAL "" FORCE  )
else( )
  tde_message_fatal( "freetype2 are required, but not found on your system" )
endif( )


##### check for fontconfig ######################

pkg_search_module( FONTCONFIG fontconfig )
if( FONTCONFIG_FOUND )
  set( HAVE_FONTCONFIG 1 CACHE INTERNAL "" FORCE )
else( )
  tde_message_fatal( "fontconfig are required, but not found on your system" )
endif( )


##### check for xft #############################

pkg_search_module( XFT xft )
if( XFT_FOUND )
  set( HAVE_XFT 1 CACHE INTERNAL "" FORCE )
else( )
  tde_message_fatal( "xft are required, but not found on your system" )
endif( )


##### check for libusb ##########################

if( WITH_LIBUSB OR ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" )
  pkg_search_module( LIBUSB libusb libusb-2.0 )
  if( LIBUSB_FOUND )
    set( HAVE_LIBUSB 1 CACHE INTERNAL "" FORCE )
  else( )
    tde_message_fatal( "libusb is required, but not found on your system" )
  endif( )
endif( )


##### check for libraw1394 ######################

if( WITH_LIBRAW1394 )
  pkg_search_module( LIBRAW1394 libraw1394 )
  if( NOT LIBRAW1394_FOUND )
    tde_message_fatal( "libraw1394 are requested, but not found on your system" )
  endif( )
endif( )


##### check for fontenc #########################

# fontenc seems unused in sources

# pkg_search_module( FONTENC fontenc )
# if( FONTENC_FOUND )
#     set( HAVE_FONT_ENC 1 CACHE INTERNAL "" FORCE )
# endif( )
