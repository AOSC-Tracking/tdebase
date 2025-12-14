#################################################
#
#  (C) 2010-2012 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  (C) 2014 Timothy Pearson
#  kb9vqf (AT) pearsoncomputing (DOT) net
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

# required stuff
tde_setup_compiler_flags( )

include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN)

tde_setup_largefiles( )

find_package( TQt )
find_package( TDE )

# strlcat and strlcpy check
check_function_exists( strlcat HAVE_STRLCAT )
check_symbol_exists( strlcat "string.h" HAVE_STRLCAT_PROTO )
check_function_exists( strlcpy HAVE_STRLCPY )
check_symbol_exists( strlcpy "string.h" HAVE_STRLCPY_PROTO )

##### look for the usb.ids file, Its location can be set EG: -DWITH_USBIDS="/opt/share/misc/usb.ids"

if( WITH_USBIDS )
    set( USBIDS_FILE "${WITH_USBIDS}" )
    message( STATUS "Using specified usb.ids file: ${USBIDS_FILE}" )
 else()
    find_file( PATH_USBIDS usb.ids
               HINTS /usr/share/misc
                     /var/lib/usbutils
                     /usr/share/hwdata
    )
    if( PATH_USBIDS )
        set( USBIDS_FILE "${PATH_USBIDS}" )
        message( STATUS "Using system usb.ids file: ${PATH_USBIDS}" )
      else()
        set( USE_BUILTIN_USBIDS 1 )
        message( STATUS "File usb.ids (hwdata) was not found on the system, using builtin" )
    endif()
endif( WITH_USBIDS )


# stdint.h (drkonqi)
if( BUILD_DRKONQI )
  check_include_file( stdint.h HAVE_STDINT_H )
endif( )


# termios.h (tdm, tdeioslave)
if( BUILD_TDM OR BUILD_TDEIOSLAVES )
  check_include_file( termios.h HAVE_TERMIOS_H )
endif( )


# sys/ioctl.h (tdeioslave/fish, kcontrol/info)
if( BUILD_TDEIOSLAVES OR BUILD_KCONTROL )
  check_include_file( sys/ioctl.h HAVE_SYS_IOCTL_H )
endif( )


# sys/types.h (drkonqi, tdeioslave/smtp)
if( BUILD_DRKONQI OR BUILD_TDEIOSLAVES )
  check_include_file( sys/types.h HAVE_SYS_TYPES_H )
endif( )


# sys/time.h (tdeioslave/sftp, ksmserver, ksplashml)
if( BUILD_KSMSERVER OR BUILD_KSPLASHML OR BUILD_TDEIOSLAVES )
  check_include_file( sys/time.h HAVE_SYS_TIME_H )
  check_include_files( "sys/time.h;time.h" TIME_WITH_SYS_TIME )
endif( )

# libssh (tdeioslave/sftp)
if( BUILD_TDEIOSLAVES AND WITH_SFTP )
  pkg_search_module( LIBSSH libssh )
  if( NOT LIBSSH_FOUND )
    tde_message_fatal( "LibSSH is required, but was not found on your system" )
  endif( )
endif( )


# pam and shadow
if( BUILD_KCHECKPASS OR BUILD_TDM )
  if ( WITH_PAM )
    check_library_exists( pam pam_start "" HAVE_PAM )
    if( HAVE_PAM )
      set( USE_PAM 1 CACHE INTERNAL "" FORCE )
      check_include_file( "security/pam_appl.h" SECURITY_PAM_APPL_H )
    endif( )
    if( HAVE_PAM AND SECURITY_PAM_APPL_H )
      set( PAM_LIBRARY pam ${CMAKE_DL_LIBS} )
    else( )
      tde_message_fatal( "pam are requested, but not found on your system" )
    endif( )
  elseif( WITH_SHADOW )
    set( HAVE_SHADOW 1 CACHE INTERNAL "" FORCE )
    set( USESHADOW 1 CACHE INTERNAL "" FORCE )
  endif( )
endif( )


# crypt
set( CRYPT_LIBRARY crypt )
check_library_exists( ${CRYPT_LIBRARY} crypt "" HAVE_CRYPT )
if( NOT HAVE_CRYPT )
  unset( CRYPT_LIBRARY )
  check_function_exists( crypt LIBC_HAVE_CRYPT )
  if( LIBC_HAVE_CRYPT )
    set( HAVE_CRYPT 1 CACHE INTERNAL "" FORCE )
  endif( LIBC_HAVE_CRYPT )
endif( NOT HAVE_CRYPT )


# tdehwlib (drkonqi, kcontrol, kicker, ksmserver, tdeioslaves, tdm)
if( WITH_TDEHWLIB )
  tde_save_and_set( CMAKE_REQUIRED_INCLUDES "${TDE_INCLUDE_DIR}" )
  check_cxx_source_compiles( "
    #include <tdemacros.h>
      #ifndef __TDE_HAVE_TDEHWLIB
      #error tdecore is not build with tdehwlib
      #endif
      int main() { return 0; } "
    HAVE_TDEHWLIB
  )
  tde_restore( CMAKE_REQUIRED_INCLUDES )
  if( NOT HAVE_TDEHWLIB )
    tde_message_fatal( "tdehwlib is required, but not built in tdecore" )
  endif( NOT HAVE_TDEHWLIB )
  set( TDEHW_LIBRARIES "tdehw-shared" )
endif( )


# udev (tsak, tdekbdledsync)
if( BUILD_TSAK OR BUILD_TDEKBDLEDSYNC )
  pkg_search_module( UDEV libudev )
  if( NOT UDEV_FOUND )
    tde_message_fatal( "udev library is required, but was not found on your system" )
  endif( )
endif( )


##### check for gcc visibility support #########

if( WITH_GCC_VISIBILITY )
  tde_setup_gcc_visibility( )
endif( )


# xrender (kdesktop, konsole, kcontrol, kicker, twin)
if( WITH_XRENDER OR BUILD_KDESKTOP OR BUILD_KONSOLE OR BUILD_KCONTROL OR BUILD_KICKER )
  pkg_search_module( XRENDER xrender )
  if( XRENDER_FOUND )
    set( HAVE_XRENDER 1 )
  elseif( WITH_XRENDER )
    tde_message_fatal( "xrender is requested, but was not found on your system" )
  endif( )
endif( )


# xrandr (kcontrol, twin/compot-tde)
if( WITH_XRANDR )
  pkg_search_module( XRANDR xrandr )
  if( NOT XRANDR_FOUND )
    tde_message_fatal( "xrandr are requested, but not found on your system" )
  endif( )
endif( )


# xinerama (ksplashml, twin/compot-tde)
if( WITH_XINERAMA )
  pkg_search_module( XINERAMA xinerama )
  if( XINERAMA_FOUND )
    set( HAVE_XINERAMA 1 )
  else( )
    tde_message_fatal( "xinerama is requested, but not found on your system" )
  endif( )
endif( WITH_XINERAMA )


# xcursor (tdeioslave, kcontrol)
if( WITH_XCURSOR )
  pkg_search_module( XCURSOR xcursor )
  if( XCURSOR_FOUND )
    set( HAVE_XCURSOR 1 )
  else( )
    tde_message_fatal( "xcursor is requested, but was not found on your system" )
  endif( )
endif( )


# xcomposite (kicker, twin)
if( WITH_XCOMPOSITE )
  pkg_search_module( XCOMPOSITE xcomposite )
  if( XCOMPOSITE_FOUND )
    set( HAVE_XCOMPOSITE 1 )
  else( XCOMPOSITE_FOUND )
    tde_message_fatal( "xcomposite is requested, but was not found on your system" )
  endif( XCOMPOSITE_FOUND )


  # xdamage (twin/kompmgr)
  pkg_search_module( XDAMAGE xdamage )
  if( NOT XDAMAGE_FOUND )
    tde_message_fatal( "xdamage is required for xcomposite support, but was not found on your system" )
  endif( )

  # xext (twin/kompmgr)
  pkg_search_module( XEXT xext )
  if( NOT XEXT_FOUND )
    tde_message_fatal( "xext is required for xcomposite support, but was not found on your system" )
  endif( )

  # for (twin/compton)
  # older libXext (e.g.in debian-6.0) doesn't provide XSyncFence
  tde_save_and_set( CMAKE_REQUIRED_INCLUDES   "${XEXT_INCLUDE_DIRS}" )
  tde_save_and_set( CMAKE_REQUIRED_LIBRARIES  "${XEXT_LIBRARIES}" )
  check_symbol_exists( "XSyncCreateFence" "X11/Xlib.h;X11/extensions/sync.h" HAVE_XEXT_XSYNCFENCE )
  tde_restore( CMAKE_REQUIRED_LIBRARIES )
  tde_restore( CMAKE_REQUIRED_INCLUDES )
endif( )


# xfixes (klipper, kicker)
if( WITH_XFIXES )
  pkg_search_module( XFIXES xfixes )
  if( XFIXES_FOUND )
    set( HAVE_XFIXES 1 CACHE INTERNAL "" FORCE )
  else( )
    tde_message_fatal( "xfixes is requested, but was not found on your system" )
  endif( )
endif( )


# libconfig (twin/compton-tde)
if( WITH_LIBCONFIG )
  pkg_search_module( LIBCONFIG libconfig )
  if( LIBCONFIG_FOUND )
    set( HAVE_LIBCONFIG 1 )
    # TODO replace with functionality check
    if( LIBCONFIG_VERSION VERSION_LESS 1.4.0 )
      set( HAVE_LIBCONFIG_OLD_API 1 )
    endif( )
  else( LIBCONFIG_FOUND )
    tde_message_fatal( "libconfig is requested, but was not found on your system" )
  endif( )
endif( )


# pcre2 (twin/compton-tde)
if( WITH_PCRE2 )
  pkg_check_modules( LIBPCRE2 libpcre2-8 libpcre2-posix )
  if( NOT LIBPCRE2_FOUND )
    tde_message_fatal( "pcre2 support was requested, but not found on your system" )
  endif( )
endif( )


# xtest (kxkb)
if( WITH_XTEST )
  pkg_search_module( XTEST xtst )
  if( XTEST_FOUND )
    set( HAVE_XTEST 1 )
  else( XTEST_FOUND )
    tde_message_fatal( "xtest is requested, but was not found on your system" )
  endif( )
endif( )


# xscreensaver ()
if( WITH_XSCREENSAVER )
  pkg_search_module( XSS xscrnsaver )

  if( XSS_FOUND )
    tde_save_and_set( CMAKE_REQUIRED_INCLUDES "${XSS_INCLUDE_DIRS}" )
    check_library_exists( Xss XScreenSaverQueryInfo "${XSS_LIBRARY_DIRS}" HAVE_XSSLIB )
    tde_restore( CMAKE_REQUIRED_INCLUDES )
  else( XSS_FOUND )
    check_library_exists( Xss XScreenSaverQueryInfo "" HAVE_XSSLIB )
  endif( XSS_FOUND )

  if( NOT HAVE_XSSLIB )
    check_library_exists( Xext XScreenSaverQueryInfo "${XEXT_LIBRARY_DIRS}" HAVE_XEXT_XSS )
    if( HAVE_XEXT_XSS )
      set( HAVE_XSSLIB 1 )
      pkg_search_module( XSS xext )
    endif( )
  endif( )

  tde_save_and_set( CMAKE_REQUIRED_INCLUDES "${XSS_INCLUDE_DIRS}" )
  check_include_file( X11/extensions/scrnsaver.h HAVE_XSCREENSAVER_H )
  tde_restore( CMAKE_REQUIRED_INCLUDES )
  if( HAVE_XSSLIB AND HAVE_XSCREENSAVER_H )
    set( HAVE_XSCREENSAVER 1 )
  else( )
    tde_message_fatal( "xscreensaver is requested, but was not found on your system" )
  endif( )

  # We don't really need the xscreensaver package for building, we only need to know
  # where xscreensaver stores its executables. So give the user the possibility
  # to define XSCREENSAVER_DIR and speficy the location manually.
  include( FindXscreensaver.cmake ) # not really good practise
  if( NOT XSCREENSAVER_DIRS )
    tde_message_fatal(
      "xscreensaver is requested, but cmake can not determine the location of XSCREENSAVER_DIRS
 You have to either specify it manually with e.g. -DXSCREENSAVER_DIRS=/usr/lib/misc/xscreensaver/
 or make sure that xscreensaver installed properly" )
  endif( )

endif( )


# openGL (kdesktop or kcontrol or tdescreensaver or twin/compot-tde )
if( WITH_OPENGL )
  pkg_search_module( GL gl )
  if( GL_FOUND )
    # some extra check, stricktly speaking they are not necessary
    tde_save_and_set( CMAKE_REQUIRED_LIBRARIES ${GL_LIBRARIES} )
    tde_save_and_set( CMAKE_REQUIRED_INCLUDES "${GL_INCLUDE_DIRS}" )
    tde_save_and_set( CMAKE_REQUIRED_FLAGS "${GL_LDFLAGS}" )
    check_symbol_exists( glXChooseVisual "GL/glx.h" HAVE_GLXCHOOSEVISUAL )
    tde_restore( CMAKE_REQUIRED_LIBRARIES )
    tde_restore( CMAKE_REQUIRED_INCLUDES )
    tde_restore( CMAKE_REQUIRED_FLAGS )
    if( NOT HAVE_GLXCHOOSEVISUAL )
      tde_message_fatal( "opengl is requested and found, but it doesn't provides glXChooseVisual() or GL/glx.h" )
    endif( )
  else( )
    check_library_exists( GL glXChooseVisual "" HAVE_GLXCHOOSEVISUAL )
    if( HAVE_GLXCHOOSEVISUAL )
      set( GL_LIBRARIES "GL" )
    else( HAVE_GLXCHOOSEVISUAL )
      tde_message_fatal( "opengl is requested, but not found on your system" )
    endif( HAVE_GLXCHOOSEVISUAL )
  endif( )

  if( BUILD_KCONTROL )
    pkg_search_module( GLU glu )
    if( NOT GLU_FOUND )
      check_library_exists( GLU gluGetString "" HAVE_GLUGETSTRING )
      if( HAVE_GLUGETSTRING )
        set( GLU_LIBRARIES "GLU" )
      else( HAVE_GLUGETSTRING )
        tde_message_fatal( "glu is required, but not found on your system" )
      endif( HAVE_GLUGETSTRING )
    endif( )
  endif( BUILD_KCONTROL )

endif( )


# glib-2.0
if( BUILD_NSPLUGINS )
  pkg_search_module( GLIB2 glib-2.0 )
  if( NOT GLIB2_FOUND )
    tde_message_fatal( "glib-2.0 are required, but not found on your system" )
  endif( )
endif( )


# kde_socklen_t
if( BUILD_TDEIOSLAVES OR BUILD_KSYSGUARD )
  set( kde_socklen_t socklen_t )
endif( )


# getifaddrs (kcontrol, tdm)
if( BUILD_KCONTROL OR BUILD_TDM )
  check_function_exists( getifaddrs HAVE_GETIFADDRS )
endif( )


# xkb (konsole, tdm, kxkb)
if( BUILD_KONSOLE OR BUILD_TDM OR BUILD_KXKB )
  check_include_file( X11/XKBlib.h HAVE_X11_XKBLIB_H )
  if( HAVE_X11_XKBLIB_H )
    check_library_exists( X11 XkbLockModifiers "" HAVE_XKB )
    if( BUILD_TDM )
      check_library_exists( X11 XkbSetPerClientControls "" HAVE_XKBSETPERCLIENTCONTROLS )
    endif( )
  endif( )
endif( )


# XBINDIR, XLIBDIR (tdm, kxkb)
if( BUILD_TDM OR BUILD_KXKB )
  find_program( some_x_program NAMES iceauth xrdb xterm )
  if( NOT some_x_program )
    set( some_x_program /usr/bin/xrdb )
    message( STATUS "Warning: Could not determine X binary directory. Assuming /usr/bin." )
  endif( )
  get_filename_component( proto_xbindir "${some_x_program}" PATH )
  get_filename_component( XBINDIR "${proto_xbindir}" ABSOLUTE )
  get_filename_component( xrootdir "${XBINDIR}" PATH )
  set( XBINDIR ${XBINDIR} CACHE INTERNAL "" FORCE )
  set( XLIBDIR "${xrootdir}/lib/X11" CACHE INTERNAL "" FORCE )
endif( )


# arts
if( WITH_ARTS )
  pkg_search_module( ARTS arts )
  if( NOT ARTS_FOUND )
    message( FATAL_ERROR "\naRts is requested, but was not found on your system" )
  endif( )
else( )
  set( WITHOUT_ARTS 1 )
endif( )


# libart
if( WITH_LIBART )
  pkg_search_module( LIBART libart-2.0 )
  if( NOT LIBART_FOUND )
    message(FATAL_ERROR "\nlibart-2.0 support are requested, but not found on your system" )
  endif( NOT LIBART_FOUND )
  set( HAVE_LIBART 1 )
endif( WITH_LIBART )


# dbus (tdm, kdesktop, twin/compton-tde.c)
if( BUILD_TDM OR BUILD_KDESKTOP OR (BUILD_TWIN AND WITH_XCOMPOSITE) )

  pkg_search_module( DBUS dbus-1 )
  if( NOT DBUS_FOUND )
    tde_message_fatal( "dbus-1 is required, but was not found on your system" )
  endif( )

endif( )


# dbus-1-tqt (kdesktop)
if( BUILD_KDESKTOP )

  pkg_search_module( DBUS_1_TQT dbus-1-tqt )
  if( NOT DBUS_1_TQT_FOUND )
    tde_message_fatal( "dbus-1-tqt is required, but was not found on your system" )
  endif( )

endif( )


# check for krb5
if( WITH_KRB5 )
  pkg_search_module( KRB5 krb5 )
  if( NOT KRB5_FOUND )
    check_include_file( kadm5/admin.h HAVE_KRB_KADM_H )
    if( NOT HAVE_KRB_KADM_H )
      message(FATAL_ERROR "\nKerberos support was requested, but krb5 was not found on your system" )
    endif( NOT HAVE_KRB_KADM_H )
  endif( NOT KRB5_FOUND )
  set( HAVE_KRB5 1 )
endif( )

# check for libr
if( WITH_ELFICON )
  pkg_search_module( LIBR libr )
  if( NOT LIBR_FOUND )
      message(FATAL_ERROR "\nelficon support was requested, but libr was not found on your system" )
  endif( NOT LIBR_FOUND )
  if( "${LIBR_VERSION}" VERSION_LESS "0.6" )
      message(FATAL_ERROR "\nelficon support was requested, but the libr version on your system may not be compatible with TDE" )
  endif( "${LIBR_VERSION}" VERSION_LESS "0.6" )
  set( HAVE_ELFICON 1 )
endif( )

#
# For kxkb, try to find the path to the Xkb rules files:
#
#   1: ask for the path from xkeyboard-config's pkg-config file
#   2: ask for the path from xkbcomp's pkg-config file
#   3: look under the "FILES" heading in the man page for setxkbmap
#   4: take the prefix/libdir from xkbfile's pkg-config file and try the
#      "${prefix}/share/X11" and "${libdir}/X11" directories
#
# Alternatively, just take an overriding value from the command line.
#

if( BUILD_KXKB )

  if( NOT X11_XKB_RULES_DIR )
    pkg_get_variable( KB_RULES_DIR xkeyboard-config xkb_base )
    if( KB_RULES_DIR )
      string(REGEX REPLACE "/xkb$" "/" X11_XKB_RULES_DIR "${KB_RULES_DIR}" )
    endif( )
  endif( )

  if( NOT X11_XKB_RULES_DIR )
    pkg_get_variable( KB_RULES_DIR xkbcomp xkbconfigdir )
    if( KB_RULES_DIR )
      string(REGEX REPLACE "/xkb$" "/" X11_XKB_RULES_DIR "${KB_RULES_DIR}" )
    endif( )
  endif( )

  if( NOT X11_XKB_RULES_DIR )
    execute_process(
      COMMAND man -P cat setxkbmap
      OUTPUT_VARIABLE SETXKBMAP_OUTPUT
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE SETXKBMAP_OUTPUT
      ERROR_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE SETXKBMAP_RC
    )
    if( "${SETXKBMAP_RC}" STREQUAL "0" AND
        "${SETXKBMAP_OUTPUT}" MATCHES "\n.*FILES.*\n[^/]*(/[^\n]*/)xkb.*\n" )
      if( EXISTS "${CMAKE_MATCH_1}xkb" )
        set( X11_XKB_RULES_DIR "${CMAKE_MATCH_1}" )
      endif( )
    endif( )
  endif( )

  if( NOT X11_XKB_RULES_DIR )
    pkg_get_variable( KB_RULES_LIBDIR xkbfile libdir)
    pkg_get_variable( KB_RULES_PREFIX xkbfile prefix)
    if( KB_RULES_LIBDIR AND KB_RULES_PREFIX )
      find_file( RULES_FILE
        NAMES xorg xfree86
        PATHS "${KB_RULES_PREFIX}/share/X11"
              "${KB_RULES_LIBDIR}/X11"
        PATH_SUFFIXES xkb/rules
        NO_DEFAULT_PATH
      )
      if( RULES_FILE )
        string( REGEX REPLACE "/xkb/rules/(xorg|xfree86)$" "/" X11_XKB_RULES_DIR "${RULES_FILE}" )
      endif( )
    endif( )
  endif( )

  if( X11_XKB_RULES_DIR )
    if( NOT "${X11_XKB_RULES_DIR}" MATCHES "/$" )
      set( X11_XKB_RULES_DIR "${X11_XKB_RULES_DIR}/" )
    endif( )
    message( STATUS "Adding ${X11_XKB_RULES_DIR} to XKb rules directory search" )
  else( )
    message( STATUS "No additional XKb rules directory found" )
  endif( )

  if( WITH_XKB_TRANSLATIONS )
    if( NOT XKB_CONFIG_LOCALE_DIR )
      pkg_get_variable( XKB_CONFIG_DATADIR xkeyboard-config datadir )
      if( XKB_CONFIG_DATADIR )
        set( XKB_CONFIG_LOCALE_DIR "${XKB_CONFIG_DATADIR}/locale" CACHE INTERNAL "" )
        message( STATUS "Found xkeybord-config locale dir: ${XKB_CONFIG_LOCALE_DIR}" )
      endif( )
    endif( )

    if( NOT XKB_CONFIG_LOCALE_DIR )
      tde_message_fatal( "Translations for xkb messages were requested but the xkeyboard-config locale directory could not be determined." )
    endif( )
  endif( )

endif( )

if( BUILD_KCONTROL OR BUILD_TDM )
  # SunOS kstat
  check_library_exists( kstat kstat_open "" HAVE_KSTAT )
  if( HAVE_KSTAT )
    set( KSTAT_LIBRARIES kstat )
  else()
    if( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )
      tde_message_fatal( "libkstat not found on SunOS platform!" )
    endif()
    set( KSTAT_LIBRARIES "" )
  endif( )

  if( NOT DEFINED REBOOT_BINARY )
    message( STATUS "Looking for reboot" )
    find_program( REBOOT_BINARY reboot
                  HINTS /sbin
                        /usr/sbin
    )
    if( REBOOT_BINARY )
      message( STATUS "Looking for reboot - ${REBOOT_BINARY}" )
    endif( REBOOT_BINARY )
  endif( NOT DEFINED REBOOT_BINARY )

  if( NOT DEFINED POWEROFF_BINARY )
    message( STATUS "Looking for poweroff" )
    find_program( POWEROFF_BINARY poweroff
                  HINTS /sbin
                        /usr/sbin
    )
    if( POWEROFF_BINARY )
      message( STATUS "Looking for poweroff - ${POWEROFF_BINARY}" )
    endif( POWEROFF_BINARY )
  endif( NOT DEFINED POWEROFF_BINARY )

  if( NOT DEFINED HALT_BINARY )
    message( STATUS "Looking for halt" )
    find_program( HALT_BINARY halt
                  HINTS /sbin
                        /usr/sbin
    )
    if( HALT_BINARY )
      message( STATUS "Looking for halt - ${HALT_BINARY}" )
    endif( HALT_BINARY )
  endif( NOT DEFINED HALT_BINARY )

  if( NOT DEFINED SHUTDOWN_BINARY )
    message( STATUS "Looking for shutdown" )
    find_program( SHUTDOWN_BINARY shutdown
                  HINTS /sbin
                        /usr/sbin
    )
    if( SHUTDOWN_BINARY )
      message( STATUS "Looking for shutdown - ${SHUTDOWN_BINARY}" )
    endif( SHUTDOWN_BINARY )
  endif( NOT DEFINED SHUTDOWN_BINARY )

  if( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )
    # SunOS based systems
    if( NOT REBOOT_BINARY AND SHUTDOWN_BINARY )
      # emulate reboot
      set( REBOOT_BINARY "${SHUTDOWN_BINARY} -y -i 6")
    endif( NOT REBOOT_BINARY AND SHUTDOWN_BINARY )

    if( NOT POWEROFF_BINARY AND SHUTDOWN_BINARY )
      # emulate poweroff
      set( POWEROFF_BINARY "${SHUTDOWN_BINARY} -y -i 5")
    endif( NOT POWEROFF_BINARY AND SHUTDOWN_BINARY )

  else( ) # default condition
    if( NOT REBOOT_BINARY AND SHUTDOWN_BINARY )
      # emulate reboot
      set( REBOOT_BINARY "${SHUTDOWN_BINARY} -r now")
    endif( NOT REBOOT_BINARY AND SHUTDOWN_BINARY )

    if( NOT POWEROFF_BINARY AND SHUTDOWN_BINARY )
      # emulate poweroff
      set( POWEROFF_BINARY "${SHUTDOWN_BINARY} -h now")
    endif( NOT POWEROFF_BINARY AND SHUTDOWN_BINARY )

    if( NOT POWEROFF_BINARY AND HALT_BINARY )
      # emulate poweroff
      set( POWEROFF_BINARY "${HALT_BINARY} -p")
    endif( NOT POWEROFF_BINARY AND HALT_BINARY )

  endif( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )

  if( NOT REBOOT_BINARY )
    tde_message_fatal( "reboot command is not defined" )
  endif( NOT REBOOT_BINARY )

  if( NOT POWEROFF_BINARY )
    tde_message_fatal( "poweroff command is not defined" )
  endif( NOT POWEROFF_BINARY )

  message( STATUS "poweroff - ${POWEROFF_BINARY}" )
  message( STATUS "reboot - ${REBOOT_BINARY}" )

endif( BUILD_KCONTROL OR BUILD_TDM )

# XInput (kcontrol/input/touchpad.cpp)
if( BUILD_KCONTROL )
  pkg_search_module( XINPUT xi )
  if( NOT XINPUT_FOUND )
    tde_message_fatal( "XInput is required, but was not found on your system" )
  endif( )
endif ( BUILD_KCONTROL )


check_include_files( "sys/time.h;sys/loadavg.h" HAVE_SYS_LOADAVG_H )
