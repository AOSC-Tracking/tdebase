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

add_subdirectory( data )
# add_subdirectory( doc )
add_subdirectory( icons )
add_subdirectory( src )


tde_create_translated_desktop(
  SOURCE konsole.desktop konsolesu.desktop
  PO_DIR konsole-desktops
)

tde_create_translated_desktop(
  SOURCE konsolepart.desktop kwrited.desktop konsole-script.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR konsole-desktops
)

tde_create_translated_desktop(
  SOURCE terminalemulator.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR konsole-desktops
)

install(
  FILES konsole.upd
  DESTINATION ${DATA_INSTALL_DIR}/tdeconf_update
)

install(
  PROGRAMS schemaStrip.pl
  DESTINATION ${DATA_INSTALL_DIR}/tdeconf_update
)

tde_create_translated_desktop(
  SOURCE kwrited.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}/kded
  PO_DIR konsole-desktops
)

tde_create_translated_desktop(
  SOURCE terminalhere.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqueror/servicemenus
  PO_DIR konsole-desktops
)

tde_create_translated_desktop(
  SOURCE eventsrc
  DESTINATION ${DATA_INSTALL_DIR}/konsole
  PO_DIR konsole-eventsrc
)

install(
  FILES tips
  DESTINATION ${DATA_INSTALL_DIR}/konsole
)
