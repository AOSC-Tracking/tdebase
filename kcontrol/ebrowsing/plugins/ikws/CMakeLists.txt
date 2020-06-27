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

add_subdirectory( searchproviders )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

install( FILES kuriikwsfilter.upd DESTINATION ${KCONF_UPDATE_INSTALL_DIR} )

tde_create_translated_desktop(
  SOURCE kuriikwsfilter.desktop kurisearchfilter.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR kcontrol-desktops
)

tde_create_translated_desktop(
  SOURCE searchprovider.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR kcontrol-desktops
)


##### libkuriikwsfilter (module) ################

tde_add_kpart( libkuriikwsfilter AUTOMOC
  SOURCES
    kuriikwsfiltereng.cpp kuriikwsfilter.skel
    kuriikwsfilter.cpp searchprovider.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### libkurisearchfilter (module) ##############

tde_add_kpart( libkurisearchfilter AUTOMOC
  SOURCES
    kurisearchfilter.cpp kurisearchfilter.skel
    kuriikwsfiltereng.cpp ikwsopts.cpp searchproviderdlg.cpp
    searchprovider.cpp searchproviderdlg_ui.ui ikwsopts_ui.ui
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)