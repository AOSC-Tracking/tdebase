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

add_definitions( -DKCMRULES )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/twin/lib
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE twinrules.desktop
  PO_DIR twin-desktops
)


##### kcm_twinrules (module) ####################

tde_add_kpart( kcm_twinrules AUTOMOC
  SOURCES kcm.cpp
  LINK twinrules-static
  DESTINATION ${PLUGIN_INSTALL_DIR} )


##### twin_rules_dialog (tdeinit) ###############

tde_add_tdeinit_executable( twin_rules_dialog
  SOURCES main.cpp
  LINK ${TQT_LIBRARIES} twinrules-static )


##### twinrules (static) ########################

set( target twinrules )

set( ${target}_SRCS
  ruleswidget.cpp ruleslist.cpp twinsrc.cpp detectwidget.cpp
  ruleswidgetbase.ui ruleslistbase.ui detectwidgetbase.ui
  editshortcutbase.ui )

tde_add_library( ${target} STATIC_PIC AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK tdeui-shared )
