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

option( WITH_DPMS "Enable DPMS power saving" ON )

if( WITH_DPMS )

  include( CheckCSourceCompiles )

  set( CMAKE_REQUIRED_LIBRARIES Xext )
  set( CMAKE_REQUIRED_INCLUDES "${XEXT_INCLUDE_DIRS}" )
  set( CMAKE_REQUIRED_FLAGS "${XEXT_LDFLAGS}" )

  check_c_source_compiles("
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <arpa/nameser.h>
    #include <resolv.h>
    #include <X11/Xproto.h>
    #include <X11/X.h>
    #include <X11/Xlib.h>
    #include <X11/extensions/dpms.h>
    int main()
    { return DPMSSetTimeouts( 0, 0, 0, 0 ); }"
    HAVE_DPMS )

  if( NOT HAVE_DPMS )
    tde_message_fatal( "DPMS support are requested, but is not detected on you system.\n libXext (and its devel package) are installed?" )
  endif()

  set( XEXT_LIBRARY Xext )

endif()

add_subdirectory( pics )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${XEXT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${XEXT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE energy.desktop
  DESTINATION ${APPS_INSTALL_DIR}/.hidden
  PO_DIR kcontrol-desktops
)


##### kcm_energy (module) #######################

tde_add_kpart( kcm_energy AUTOMOC
  SOURCES energy.cpp
  LINK tdeio-shared
  LINK_PRIVATE ${XEXT_LIBRARY}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
