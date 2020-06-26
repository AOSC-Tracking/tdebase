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

add_subdirectory( pixmaps )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_install_icons( kmenuedit )
tde_create_translated_desktop( kmenuedit.desktop )
install( FILES uninstall.desktop DESTINATION ${APPS_INSTALL_DIR}/System RENAME kmenuedit.desktop )
install( FILES kmenueditui.rc DESTINATION ${DATA_INSTALL_DIR}/kmenuedit )
install( FILES kcontroleditui.rc DESTINATION ${DATA_INSTALL_DIR}/kcontroledit )


##### kmenuedit (tdeinit) #######################

tde_add_tdeinit_executable( kmenuedit AUTOMOC
  SOURCES main.cpp
  LINK ${TQT_LIBRARIES} kmenueditcommon-static
)


##### kcontroledit (tdeinit) ####################

tde_add_tdeinit_executable( kcontroledit AUTOMOC
  SOURCES kcontrol_main.cpp
  LINK ${TQT_LIBRARIES} kmenueditcommon-static
)


##### kmenueditcommon (static) ##################

tde_add_library( kmenueditcommon STATIC_PIC AUTOMOC
  SOURCES
    basictab.cpp treeview.cpp kmenuedit.cpp
    khotkeys.cpp menufile.cpp menuinfo.cpp
  LINK tdeio-shared
)
