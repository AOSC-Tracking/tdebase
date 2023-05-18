#################################################
#
#  (C) 2023 DilOS Team
#  denis (AT) tambov (DOT) ru
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

# devinfo
check_library_exists( devinfo di_init "" HAVE_DEVINFO )
if( HAVE_DEVINFO )
  set( DEVINFO_LIBRARIES devinfo )
else()
  if( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )
    tde_message_fatal( "libdevinfo not found on SunOS platform!" )
  endif()
  set( DEVINFO_LIBRARIES "" )
endif( )
