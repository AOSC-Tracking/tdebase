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

project( libkonq )

check_include_file( "sys/statvfs.h" HAVE_STATVFS )

add_subdirectory( pics )
add_subdirectory( favicons )
add_subdirectory( servicemenus )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${ARTS_INCLUDE_DIRS}
  ${TDE_INCLUDE_DIR}/arts/
  ${CMAKE_BINARY_DIR}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${ARTS_LIBRARY_DIRS}
)


##### headers ###################################

install( FILES
    konq_popupmenu.h knewmenu.h tdefileivi.h konq_drag.h
    konq_iconviewwidget.h konq_defaults.h konq_settings.h
    konq_operations.h libkonq_export.h konq_dirpart.h
    konq_propsview.h konq_events.h konq_undo.h
    konq_historymgr.h konq_historycomm.h konq_pixmapprovider.h
    kivdirectoryoverlay.h kivfreespaceoverlay.h konq_faviconmgr.h
    konq_xmlguiclient.h konqbookmarkmanager.h konq_filetip.h
    konq_sort_constants.h konq_string_compare.h
  DESTINATION ${INCLUDE_INSTALL_DIR} )


##### other data ################################

tde_create_translated_desktop(
  SOURCE directory_bookmarkbar.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kbookmark
  PO_DIR libkonq-desktops
)

tde_create_translated_desktop(
  SOURCE konqpopupmenuplugin.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR libkonq-desktops
)


##### konq (shared lib) #########################

tde_add_library( konq SHARED AUTOMOC
  SOURCES
    konq_popupmenu.cpp knewmenu.cpp konq_xmlguiclient.cpp tdefileivi.cpp
    konq_iconviewwidget.cpp konq_settings.cpp konq_drag.cpp konq_operations.cpp
    konq_dirpart.cpp konq_propsview.cpp konq_events.cpp konq_bgnddlg.cpp konq_undo.cpp
    konq_undo.skel konq_historymgr.cpp konq_historycomm.cpp konq_historycomm.skel
    konq_pixmapprovider.cpp kivdirectoryoverlay.cpp kivfreespaceoverlay.cpp
    konq_faviconmgr.cpp konq_faviconmgr.skel konq_filetip.cpp
  VERSION 4.2.0
  LINK tdeparts-shared
  DESTINATION ${LIB_INSTALL_DIR}
)


##### konq_sound (module) #######################

if( WITH_ARTS )
  tde_add_kpart( konq_sound
    SOURCES konq_sound.cpp
    LINK artskde-shared
    DESTINATION ${PLUGIN_INSTALL_DIR} )
endif( )


##### install import cmake modules ###############

tde_install_export( )
