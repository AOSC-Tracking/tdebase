if( WITH_SUDO_KONSOLE_SUPER_USER_COMMAND )
  set( KONSOLE_SUPER_USER_COMMAND "sudo su" CACHE INTERNAL "" FORCE )
else()
  set( KONSOLE_SUPER_USER_COMMAND su CACHE INTERNAL "" FORCE )
endif()

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
