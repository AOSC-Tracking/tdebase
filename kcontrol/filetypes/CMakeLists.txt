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

tde_create_translated_desktop(
  SOURCE filetypes.desktop
  PO_DIR kcontrol-desktops
)


##### kcm_filetypes (module) ####################

tde_add_kpart( kcm_filetypes AUTOMOC
  SOURCES control.cpp
  LINK filetypes-static tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### filetypes (static) ########################

tde_add_library( filetypes STATIC_PIC AUTOMOC
  SOURCES
    filetypesview.cpp filetypedetails.cpp filegroupdetails.cpp
    kservicelistwidget.cpp typeslistitem.cpp newtypedlg.cpp
    kserviceselectdlg.cpp
)

##### keditfiletype (executable) ######################

tde_add_executable( keditfiletype AUTOMOC
  SOURCES keditfiletype.cpp
  LINK filetypes-static tdeio-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
