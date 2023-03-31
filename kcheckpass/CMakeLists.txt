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

include( ConfigureChecks.cmake )

if( HAVE_PAM )
  add_definitions( -DHAVE_PAM )
endif( )

include_directories(
  ${CMAKE_BINARY_DIR}
)


##### kcheckpass (executable) ###################

tde_add_executable( kcheckpass AUTOMOC
  SOURCES
    kcheckpass.c checkpass_pam.c
    checkpass_shadow.c checkpass_osfc2passwd.c checkpass_aix.c
  LINK tdefakes-shared ${CRYPT_LIBRARY} ${PAM_LIBRARY}
  DESTINATION ${BIN_INSTALL_DIR}
  SETUID
)

