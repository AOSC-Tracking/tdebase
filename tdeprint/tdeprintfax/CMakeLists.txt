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

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_install_icons( tdeprintfax )
tde_install_icons( abentry DESTINATION ${DATA_INSTALL_DIR}/tdeprintfax/icons )

tde_create_translated_desktop(
  SOURCE tdeprintfax.desktop
  PO_DIR tdeprint-desktops
)

install( FILES tdeprintfaxui.rc faxfilters DESTINATION ${DATA_INSTALL_DIR}/tdeprintfax )
install( PROGRAMS anytops DESTINATION ${DATA_INSTALL_DIR}/tdeprintfax )


##### tdeprintfax (executable) ##################

tde_add_executable( tdeprintfax AUTOMOC
  SOURCES
    main.cpp tdeprintfax.cpp faxab.cpp faxctrl.cpp
    confgeneral.cpp configdlg.cpp conffax.cpp
    confsystem.cpp conffilters.cpp filterdlg.cpp
    defcmds.cpp
  LINK tdeabc-shared tdeprint-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
