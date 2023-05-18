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

add_subdirectory( pics )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR} # for libkonq/konq_defaults.h
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_install_icons( )
tde_create_translated_desktop( kpersonalizer.desktop )
install( FILES uninstall.desktop DESTINATION ${APPS_INSTALL_DIR}/System RENAME kpersonalizer.desktop )


##### kpersonalizer (executable) ################

tde_add_executable( kpersonalizer AUTOMOC
  SOURCES
    stylepreview.ui krefinepage.cpp tdestylepage.cpp
    keyecandypage.cpp kospage.cpp kcountrypage.cpp
    kpersonalizer.cpp main.cpp kfindlanguage.cpp
    kcountrypagedlg.ui kospagedlg.ui keyecandypagedlg.ui
    tdestylepagedlg.ui krefinepagedlg.ui ksysinfo.cpp
  LINK tdeio-shared ${KSTAT_LIBRARIES}
  DESTINATION ${BIN_INSTALL_DIR}
)
