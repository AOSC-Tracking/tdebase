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

add_subdirectory( icons )
add_subdirectory( wallpapers )

tde_install_icons( )

if( WITH_SUDO_KONSOLE_SUPER_USER_COMMAND )
  set( KONSOLE_SUPER_USER_COMMAND "sudo su" CACHE INTERNAL "" FORCE )
else()
  set( KONSOLE_SUPER_USER_COMMAND su CACHE INTERNAL "" FORCE )
endif()

install(
  FILES
    BlackOnLightColor.schema BlackOnLightYellow.schema
    BlackOnWhite.schema Example.Schema GreenOnBlack.schema
    WhiteOnBlack.schema README.default.Schema syscolor.schema
    Linux.schema XTerm.schema vim.schema DarkPicture.schema
    LightPicture.schema Transparent.schema GreenTint.schema
    Transparent_MC.schema GreenTint_MC.schema Transparent_lightbg.schema
    Transparent_darkbg.schema LightPaper.schema
    linux.keytab vt100.keytab vt420pc.keytab x11r5.keytab
    solaris.keytab README.Schema README.KeyTab README.default.Keytab
  DESTINATION ${DATA_INSTALL_DIR}/konsole
)

tde_create_translated_desktop(
  SOURCE
    linux.desktop
    mc.desktop
    shell.desktop
    screen.desktop
    su.desktop.cmake
    sumc.desktop.cmake
  DESTINATION ${DATA_INSTALL_DIR}/konsole
  PO_DIR konsole-desktops
)

tde_create_translated_desktop(
  SOURCE
    2shells 3shells 4shells 5shells
  DESTINATION ${DATA_INSTALL_DIR}/konsole/profiles
  KEYWORDS Title0 Title1 Title2 Title3 Title4 Title5
  PO_DIR konsole-desktops
)
